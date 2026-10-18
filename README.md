# GRO Q 4

Ecualizador paramétrico de hasta 24 bandas con EQ dinámico, hecho con JUCE 8 + CMake.
Formatos: VST3 (Windows, macOS, Linux), AU (macOS), Standalone. Binario universal en macOS (arm64 + x86_64).

## Funciones (v1)
- 24 bandas: Bell, Low/High Shelf, Low/High Cut (6 a 96 dB/oct), Notch, Band Pass, Tilt Shelf, All Pass.
- Por banda: Freq, Gain, Q, canal (Stereo / Left / Right / Mid / Side).
- EQ dinámico por banda (Bell y Shelf): Threshold, Attack, Release, Range, con sidechain interno o externo.
- Carácter: Clean / Subtle / Warm. Auto Gain, Gain Scale, Output Gain.
- Analizador de espectro (entrada y salida), medidor de salida, rango visual 3/6/12/30 dB.
- Latencia cero. Mono y estéreo. Bus de sidechain opcional.

## Importar presets de FabFilter Pro-Q (.ffp)
Boton **Import .ffp** (barra superior). Lee los .ffp de texto de Pro-Q 4 (cabecera `FQ4p`): bandas, forma, frecuencia (2^valor Hz), ganancia, Q, pendiente, canal, dinamica basica, caracter, output y Gain Scale.
Supuestos sin confirmar: el mapa de pendientes (indice 2 = 12 dB/oct) y la conversion de Threshold/Attack/Release. Natural/Linear Phase y Spectral Dynamics se avisan pero no se reproducen.

## Uso de la interfaz
- Doble clic en vacío: crear banda. Doble clic en un nodo: borrarla.
- Arrastrar nodo: frecuencia y ganancia. Rueda sobre un nodo: Q. Clic derecho: forma, canal, dinámica, borrar.

## Pendiente para próximas versiones
Natural Phase, Linear Phase, Spectral Dynamics, EQ Sketch, Instance List, EQ Match, Spectrum Grab, surround, Piano Roll.

## Compilar en local
```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```
Linux necesita: libasound2-dev libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libgl1-mesa-dev.
Con poca RAM usa `--parallel 2`.

## Compilar con GitHub Actions y publicar un release
1. Sube el proyecto a un repositorio de GitHub (rama `main`). Cada push compila en Linux, Windows y macOS y sube los artefactos.
2. Para publicar: `git tag v1.0.0 && git push origin v1.0.0` (crea el Release con un zip por plataforma).
Las ejecuciones manuales sobre `main` no publican release (es lo esperado).

## macOS: plugins sin firmar ni notarizar
Los binarios llevan firma ad-hoc. Si macOS los bloquea:
```
xattr -dr com.apple.quarantine "/Library/Audio/Plug-Ins/VST3/GRO Q 4.vst3"
codesign --force --deep -s - "/Library/Audio/Plug-Ins/VST3/GRO Q 4.vst3"
```
(igual para `/Library/Audio/Plug-Ins/Components/GRO Q 4.component`).

## Licencia
JUCE es AGPLv3 o de pago. Para vender el plugin sin publicar el código necesitas una licencia comercial de JUCE.
Nombre, gráficos y presets son originales; no se copia nada del plugin de referencia.
