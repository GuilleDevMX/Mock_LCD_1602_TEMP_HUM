#!/bin/bash
# ==============================================================================
# Pipeline de Compilación Nios V/c - Arquitectura Determinista IIoT
# Descripción: Genera el BSP (Bare-metal), configura CMake, compila MISRA C
#              y despliega el archivo HEX para Quartus Prime Standard.
# ==============================================================================

# 1. Resolución Dinámica de Rutas (Independiente del directorio de ejecución)
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

# Subimos DOS niveles lógicos: de sw/scripts -> sw -> raíz del proyecto
ROOT_DIR="$( cd "${SCRIPT_DIR}/../.." &> /dev/null && pwd )"

HW_DIR="${ROOT_DIR}/hw"
SW_DIR="${ROOT_DIR}/sw"
APP_DIR="${SW_DIR}/app"
SYS_NAME="system_soc"
ELF_TARGET="firmware.elf"

# Rutas de archivos clave
SOPCINFO_FILE="${HW_DIR}/ip/${SYS_NAME}.sopcinfo"
BSP_DIR="${SW_DIR}/bsp"
BUILD_DIR="${SW_DIR}/build"
BSP_SETTINGS="${BSP_DIR}/settings.bsp"

# El HEX debe ir a la raíz, donde Quartus busca 'onchip_mem.hex' durante el Assembler
HEX_OUTPUT="${ROOT_DIR}/onchip_mem.hex" 

echo "[INFO] ========================================================"
echo "[INFO] Iniciando Pipeline de Misión Crítica Nios V (RISC-V)"
echo "[INFO] Directorio Raíz: ${ROOT_DIR}"
echo "[INFO] ========================================================"

# 2. Generar Board Support Package (BSP)
echo "[INFO] Paso 1: Generando BSP a partir de ${SOPCINFO_FILE}..."
mkdir -p "${BSP_DIR}"

# Flags estrictos para Nios V Bare-Metal (Polling Architecture)
niosv-bsp -c -t=hal --sopcinfo="${SOPCINFO_FILE}" \
    --cmd="set_setting hal.sys_clk_timer none" \
    --cmd="set_setting hal.timestamp_timer none" \
    --cmd="set_setting hal.enable_reduced_device_drivers true" \
    --cmd="set_setting hal.enable_lightweight_device_driver_api true" \
    --cmd="set_setting hal.enable_c_plus_plus false" \
    --cmd="set_setting hal.enable_exit false" \
    --cmd="set_setting hal.enable_clean_exit false" \
    "${BSP_SETTINGS}"

if [ $? -ne 0 ]; then
    echo "[ERROR] Fallo en la generación del BSP. Revisa si el Nios V Command Shell está activo y si el .sopcinfo existe."
    exit 1
fi

# 3. Configurar el Proyecto CMake
echo "[INFO] Paso 2: Configurando el proyecto CMake de la aplicación..."
mkdir -p "${BUILD_DIR}"

niosv-app \
    --app-dir="${BUILD_DIR}" \
    --bsp-dir="${BSP_DIR}" \
    --srcs-recursive="${APP_DIR}/src" \
    --incs-recursive="${APP_DIR}/inc" \
    --elf-name="${ELF_TARGET}"

if [ $? -ne 0 ]; then
    echo "[ERROR] Fallo en la configuración de niosv-app."
    exit 1
fi

# 4. Compilar Firmware MISRA C
echo "[INFO] Paso 3: Compilando el firmware (Optimizaciones de Release -Os)..."
cd "${BUILD_DIR}" || exit 1
cmake -DCMAKE_BUILD_TYPE=Release .
make

if [ $? -ne 0 ]; then
    echo "[ERROR] Fallo en la compilación del código C. Revisa la sintaxis en ${APP_DIR}/src."
    exit 1
fi

# 5. Generar Archivo HEX para Quartus
echo "[INFO] Paso 4: Extrayendo binario y generando ${HEX_OUTPUT}..."
# Nota: --end=0x7FFF asume 32768 bytes (32KB) configurados en el Platform Designer
elf2hex --input="${ELF_TARGET}" --output="${HEX_OUTPUT}" --width=32 --base=0x0000 --end=0x7FFF

if [ $? -eq 0 ]; then
    echo "[ÉXITO] Pipeline completado con éxito."
    echo "[ÉXITO] Archivo HEX depositado en: ${HEX_OUTPUT}"
    echo "[ACCIÓN REQUERIDA] Ejecuta la compilación de Quartus (Assembler) para incrustar el firmware en la RAM."
else
    echo "[ERROR] Fallo en la conversión de ELF a HEX."
    exit 1
fi