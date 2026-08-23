import { cpSync, existsSync, mkdirSync, readFileSync, readdirSync, rmSync, statSync, writeFileSync } from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { spawnSync } from 'node:child_process';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const repoRoot = path.resolve(__dirname, '..');

const fqbn = 'arduino:avr:nano';
const boardSuffix = 'nano';
const packageName = 'DCS-BIOS-EasyMode-Mosquito-Nano-Firmware';
const sketches = [
  'examples\\7_Mosquito_Fuel_Panel',
  'examples\\61_Mosquito_Throttle_Quadrant',
  'examples\\62_Mosquito_Trim'
];

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
      'Compress-Archive',
      `-Path ${powershellQuote(packagePath)}`,
      `-DestinationPath ${powershellQuote(archivePath)}`,
      '-Force'
    ].join(' ');

    runCommand('powershell', ['-NoProfile', '-Command', command]);
    return;
  }

  runCommand('zip', ['-r', archivePath, packageDirName], { cwd: distDir });
}

function removePath(targetPath, options = {}) {
  rmSync(targetPath, {
    recursive: false,
    force: true,
    maxRetries: 10,
    retryDelay: 200,
    ...options
  });
}

function findFirstFileByExtension(rootDir, extension) {
  const stack = [rootDir];

  while (stack.length > 0) {
    const currentDir = stack.pop();
    for (const entry of readdirSync(currentDir, { withFileTypes: true })) {
      const entryPath = path.join(currentDir, entry.name);

      if (entry.isDirectory()) {
        stack.push(entryPath);
        continue;
      }

      if (entry.name.toLowerCase().endsWith(extension)) {
        return entryPath;
      }
    }
  }

  return '';
}

function writePackageReadme(packageDir, version) {
  writeFileSync(path.join(packageDir, 'README.txt'), [
    'DCS-BIOS EasyMode Mosquito Nano Firmware',
    'https://github.com/wotupfoo/dcs-bios-arduino-easymode',
    `Version: ${version}`,
    '',
    'This package contains prebuilt Arduino Nano firmware for Mosquito cockpit controls.',
    'It is only for burning firmware. It does not include the source library or Arduino IDE setup instructions.',
    '',
    'Requirements:',
    '- Arduino Nano connected by USB.',
    '- avrdude.exe installed and available in PATH.',
    '- The Nano COM port, such as COM6.',
    '',
    'Usage:',
    '  flash_nano.bat COM6 7_Mosquito_Fuel_Panel_nano',
    '  flash_nano.bat COM6 61_Mosquito_Throttle_Quadrant_nano',
    '  flash_nano.bat COM6 62_Mosquito_Trim_nano',
    '',
    'For old Nano bootloader boards, add old as the third argument:',
    '  flash_nano.bat COM6 61_Mosquito_Throttle_Quadrant_nano old',
    '',
    'Firmware files:',
    '- hex/*.hex files are used for flashing.',
    '- elf/*.elf files are included for symbol/debug inspection.',
    '- eep/*.eep files are included when Arduino emits EEPROM images.',
    ''
  ].join('\r\n'));
}

function writeFlashScript(packageDir) {
  writeFileSync(path.join(packageDir, 'flash_nano.bat'), [
    '@echo off',
    'setlocal EnableExtensions',
    '',
    'if "%~1"=="" goto :usage',
    'if "%~2"=="" goto :usage',
    '',
    'set "PORT=%~1"',
    'set "FIRMWARE=%~2"',
    'set "BAUD=115200"',
    'if /I "%~3"=="old" set "BAUD=57600"',
    'set "HEX=%~dp0hex\\%FIRMWARE%.hex"',
    '',
    'if not exist "%HEX%" (',
    '    echo Firmware not found: "%HEX%"',
    '    exit /b 1',
    ')',
    '',
    'where avrdude.exe >nul 2>nul',
    'if errorlevel 1 (',
    '    echo avrdude.exe was not found in PATH.',
    '    exit /b 1',
    ')',
    '',
    'avrdude.exe -patmega328p -carduino -P"%PORT%" -b%BAUD% -D -Uflash:w:"%HEX%":i',
    'exit /b %ERRORLEVEL%',
    '',
    ':usage',
    'echo Usage: %~nx0 COMx firmware_name [old]',
    'echo Example: %~nx0 COM6 61_Mosquito_Throttle_Quadrant_nano',
    'echo Example old bootloader: %~nx0 COM6 61_Mosquito_Throttle_Quadrant_nano old',
    'exit /b 2',
    ''
  ].join('\r\n'));
}

const libraryProperties = readLibraryProperties(path.join(repoRoot, 'library.properties'));
const version = libraryProperties.version || '0.0.0';
const distDir = path.join(repoRoot, 'dist');
const packageDir = path.join(distDir, packageName);
const archivePath = path.join(distDir, `${packageName}-${version}.zip`);
const cleanOnly = process.argv.includes('--clean');

removePath(packageDir, { recursive: true, force: true });
removePath(archivePath, { force: true });

if (cleanOnly) {
  console.log(`Removed ${path.relative(repoRoot, packageDir)} and ${path.relative(repoRoot, archivePath)}`);
  process.exit(0);
}

mkdirSync(path.join(packageDir, 'hex'), { recursive: true });
mkdirSync(path.join(packageDir, 'elf'), { recursive: true });
mkdirSync(path.join(packageDir, 'eep'), { recursive: true });

for (const sketch of sketches) {
  const sketchPath = path.join(repoRoot, sketch);
  const sketchName = path.basename(sketchPath);
  const buildPath = path.join(repoRoot, '.arduino-build', sketchName, 'compile');
  const outputName = `${sketchName}_${boardSuffix}`;

  if (!existsSync(sketchPath) || !statSync(sketchPath).isDirectory()) {
    console.error(`Missing sketch directory: ${sketch}`);
    process.exit(1);
  }

  removePath(buildPath, { recursive: true, force: true });
  runCommand('cmd', ['/c', 'build-arduino-cli.bat', sketch, fqbn]);

  const hexPath = findFirstFileByExtension(buildPath, '.hex');
  const elfPath = findFirstFileByExtension(buildPath, '.elf');
  const eepPath = findFirstFileByExtension(buildPath, '.eep');

  if (!hexPath || !elfPath) {
    console.error(`Missing build output for ${sketchName}`);
    process.exit(1);
  }

  cpSync(hexPath, path.join(packageDir, 'hex', `${outputName}.hex`));
  cpSync(elfPath, path.join(packageDir, 'elf', `${outputName}.elf`));
  if (eepPath) {
    cpSync(eepPath, path.join(packageDir, 'eep', `${outputName}.eep`));
  }
}

writePackageReadme(packageDir, version);
writeFlashScript(packageDir);
createZip(archivePath, packageName, distDir);

console.log(`Created ${path.relative(repoRoot, archivePath)}`);
