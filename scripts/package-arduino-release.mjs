import { cpSync, existsSync, mkdtempSync, mkdirSync, readFileSync, readdirSync, renameSync, rmSync, statSync } from 'node:fs';
import path from 'node:path';
import os from 'node:os';
import { fileURLToPath } from 'node:url';
import { spawnSync } from 'node:child_process';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..');

const cleanOnly = process.argv.includes('--clean');
const installOnly = process.argv.includes('--install');
const configFileArg = process.argv.find((arg) => arg.startsWith('--config-file='));
const defaultArduinoConfig = process.env.ARDUINO_CONFIG_FILE || path.join(repoRoot, 'arduino-cli.yaml');
const arduinoConfigFile = (configFileArg ? configFileArg.slice('--config-file='.length) : defaultArduinoConfig).replace(/^"(.*)"$/, '$1');

function readLibraryProperties(filePath) {
  const properties = {};
  const lines = readFileSync(filePath, 'utf8').split(/\r?\n/);

  for (const line of lines) {
    if (!line || line.trim().startsWith('#')) continue;

    const separatorIndex = line.indexOf('=');
    if (separatorIndex === -1) continue;

    const key = line.slice(0, separatorIndex).trim();
    const value = line.slice(separatorIndex + 1).trim();
    properties[key] = value;
  }

  return properties;
}

function readConfigValue(filePath, keyName) {
  if (!existsSync(filePath)) return '';

  const lines = readFileSync(filePath, 'utf8').split(/\r?\n/);
  for (const line of lines) {
    const trimmed = line.trim();
    if (!trimmed || trimmed.startsWith('#')) continue;

    const separatorIndex = line.indexOf(':');
    if (separatorIndex === -1) continue;

    const key = line.slice(0, separatorIndex).trim();
    const value = line.slice(separatorIndex + 1).trim().replace(/^"(.*)"$/, '$1');
    if (key === keyName) return value;
  }

  return '';
}

function resolveSketchbookDir() {
  return readConfigValue(arduinoConfigFile, 'user') || readConfigValue(arduinoConfigFile, 'directories.user');
}

function runCommand(command, args, options = {}) {
  const result = spawnSync(command, args, {
    cwd: repoRoot,
    stdio: 'inherit',
    shell: false,
    ...options
  });

  if (result.error) {
    throw result.error;
  }

  if (result.status !== 0) {
    if (options.allowFailure) {
      return result;
    }

    process.exit(result.status ?? 1);
  }

  return result;
}

function powershellQuote(value) {
  return `'${String(value).replace(/'/g, "''")}'`;
}

function createZip(archivePath, packageDirName, distDir) {
  if (process.platform === 'win32') {
    const packagePath = path.join(distDir, packageDirName);
    const command = [
      `Compress-Archive`,
      `-Path ${powershellQuote(packagePath)}`,
      `-DestinationPath ${powershellQuote(archivePath)}`,
      `-Force`
    ].join(' ');

    runCommand('powershell', ['-NoProfile', '-Command', command]);
    return;
  }

  runCommand('zip', ['-r', archivePath, packageDirName], { cwd: distDir });
}

function removeJunkFiles(rootDir) {
  const junkNames = new Set(['.DS_Store', 'Thumbs.db', 'desktop.ini']);

  function walk(currentDir) {
    for (const entry of readdirSync(currentDir, { withFileTypes: true })) {
      const entryPath = path.join(currentDir, entry.name);

      if (entry.isDirectory()) {
        walk(entryPath);
        continue;
      }

      if (junkNames.has(entry.name)) {
        rmSync(entryPath, { force: true });
      }
    }
  }

  if (statSync(rootDir).isDirectory()) {
    walk(rootDir);
  }
}

function safeRemove(targetPath, options) {
  try {
    rmSync(targetPath, {
      maxRetries: 10,
      retryDelay: 200,
      ...options
    });
    return true;
  } catch (error) {
    if (error && (error.code === 'EPERM' || error.code === 'EBUSY' || error.code === 'ENOTEMPTY' || error.code === 'EACCES')) {
      console.warn(`Warning: could not remove ${targetPath}`);
      return false;
    }

    throw error;
  }
}

function safeRename(fromPath, toPath) {
  try {
    rmSync(toPath, {
      recursive: true,
      force: true,
      maxRetries: 10,
      retryDelay: 200
    });
    renameSync(fromPath, toPath);
    return true;
  } catch (error) {
    if (error && (error.code === 'EPERM' || error.code === 'EBUSY' || error.code === 'ENOTEMPTY' || error.code === 'EACCES')) {
      console.warn(`Warning: could not rename ${fromPath}`);
      return false;
    }

    throw error;
  }
}

function timestampForFileName() {
  return new Date().toISOString().replace(/[:.]/g, '-');
}

function resolveArduinoCli() {
  if (process.env.ARDUINO_CLI) {
    return process.env.ARDUINO_CLI.replace(/^"(.*)"$/, '$1');
  }

  const candidates = [
    process.env.LOCALAPPDATA && path.join(process.env.LOCALAPPDATA, 'Programs', 'Arduino IDE', 'resources', 'app', 'lib', 'backend', 'resources', 'arduino-cli.exe'),
    process.env.ProgramFiles && path.join(process.env.ProgramFiles, 'Arduino IDE', 'resources', 'app', 'lib', 'backend', 'resources', 'arduino-cli.exe'),
    process.env['ProgramFiles(x86)'] && path.join(process.env['ProgramFiles(x86)'], 'Arduino IDE', 'resources', 'app', 'lib', 'backend', 'resources', 'arduino-cli.exe'),
    'E:\\Program Files\\Arduino IDE\\resources\\app\\lib\\backend\\resources\\arduino-cli.exe'
  ].filter(Boolean);

  for (const candidate of candidates) {
    if (existsSync(candidate)) {
      return candidate;
    }
  }

  return 'arduino-cli';
}

function installIntoSketchbook(archivePath, libraryFolderName) {
  const arduinoCli = resolveArduinoCli();
  const sketchbookDir = resolveSketchbookDir();
  const installArchivePath = path.join(stagingRoot, path.basename(archivePath));
  let librariesDir = '';
  let installTargetDir = '';
  let backupInstallDir = '';

  if (sketchbookDir) {
    librariesDir = path.join(sketchbookDir, 'libraries');
    installTargetDir = path.join(librariesDir, libraryFolderName);
    backupInstallDir = path.join(librariesDir, `${libraryFolderName}.previous-${timestampForFileName()}`);

    if (existsSync(librariesDir)) {
      for (const entry of readdirSync(librariesDir, { withFileTypes: true })) {
        if (!entry.isDirectory()) continue;
        if (!entry.name.startsWith(`${libraryFolderName}.new-`) && !entry.name.startsWith(`${libraryFolderName}.previous-`)) continue;

        safeRemove(path.join(librariesDir, entry.name), { recursive: true, force: true });
      }
    }

    if (existsSync(installTargetDir)) {
      const renamed = safeRename(installTargetDir, backupInstallDir);
      if (!renamed) {
        console.error(`Could not move the existing library out of the way at ${installTargetDir}`);
        console.error('Close Arduino IDE, Explorer windows, or other tools using that library and try again.');
        process.exit(1);
      }
    }
  }

  cpSync(archivePath, installArchivePath, { force: true });

  console.log(`Arduino CLI: ${arduinoCli}`);
  console.log(`Arduino CLI config: ${arduinoConfigFile}`);
  console.log(`Installing ${libraryFolderName} from ${installArchivePath}`);

  const installResult = runCommand(arduinoCli, [
    '--config-file',
    arduinoConfigFile,
    'lib',
    'install',
    '--zip-path',
    installArchivePath
  ], {
    allowFailure: true,
    env: {
      ...process.env,
      ARDUINO_LIBRARY_ENABLE_UNSAFE_INSTALL: 'true'
    }
  });

  if (installResult.status !== 0) {
    if (backupInstallDir && !existsSync(installTargetDir) && existsSync(backupInstallDir)) {
      safeRename(backupInstallDir, installTargetDir);
    }

    process.exit(installResult.status ?? 1);
  }

  if (backupInstallDir) {
    safeRemove(backupInstallDir, { recursive: true, force: true });
  }
}

const libraryProperties = readLibraryProperties(path.join(repoRoot, 'library.properties'));
const libraryName = libraryProperties.name || 'arduino-library';
const libraryVersion = libraryProperties.version || '0.0.0';
const safeName = libraryName.replace(/\s+/g, '-');
const archiveName = `${safeName}-${libraryVersion}-arduino.zip`;
const distDir = path.join(repoRoot, 'dist');
const legacyPackageDir = path.join(distDir, libraryName);
const stagingRoot = mkdtempSync(path.join(os.tmpdir(), `${safeName}-release-`));
const packageDir = path.join(stagingRoot, libraryName);
let archivePath = path.join(distDir, archiveName);

safeRemove(legacyPackageDir, { recursive: true, force: true });
const removedArchive = safeRemove(archivePath, { force: true });

if (existsSync(archivePath) && removedArchive === false) {
  archivePath = path.join(distDir, `${safeName}-${libraryVersion}-arduino-${timestampForFileName()}.zip`);
}

if (cleanOnly) {
  safeRemove(stagingRoot, { recursive: true, force: true });
  console.log(`Removed ${path.relative(repoRoot, legacyPackageDir)} and ${path.relative(repoRoot, archivePath)}`);
  process.exit(0);
}

mkdirSync(path.join(packageDir, 'documentation'), { recursive: true });

cpSync(path.join(repoRoot, 'src'), path.join(packageDir, 'src'), { recursive: true });
cpSync(path.join(repoRoot, 'examples'), path.join(packageDir, 'examples'), { recursive: true });
cpSync(path.join(repoRoot, 'library.properties'), path.join(packageDir, 'library.properties'));
cpSync(path.join(repoRoot, 'keywords.txt'), path.join(packageDir, 'keywords.txt'));
cpSync(path.join(repoRoot, 'README.md'), path.join(packageDir, 'README.md'));
cpSync(path.join(repoRoot, 'LICENSE'), path.join(packageDir, 'LICENSE'));
cpSync(
  path.join(repoRoot, 'documentation', 'Example_Guide.md'),
  path.join(packageDir, 'documentation', 'Example_Guide.md')
);

removeJunkFiles(packageDir);

mkdirSync(distDir, { recursive: true });
createZip(archivePath, libraryName, stagingRoot);
if (installOnly) {
  installIntoSketchbook(archivePath, libraryName);
}
safeRemove(stagingRoot, { recursive: true, force: true });

console.log(`Created ${path.relative(repoRoot, archivePath)}`);
