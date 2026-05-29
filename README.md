# MusiVST Guitar Host

**MusiVST Guitar Host** es una aplicación de escritorio nativa y portable para Windows creada con **C++17, JUCE y CMake**.

Su objetivo es simple:

```text
Guitarra → Interfaz de audio → MusiVST Guitar Host → Plugin VST3 → Salida estéreo
```

La versión **0.1** está pensada para probar guitarra eléctrica en vivo usando una interfaz de audio externa y un plugin VST3 de guitarra.

No usa web app, Electron, Tauri, backend, login, nube ni Firebase. Descargas el ZIP portable, lo descomprimes y abres el `.exe`. Qué milagro: un programa que intenta hacer una sola cosa.

---

## Qué incluye la versión 0.1

- Aplicación standalone con JUCE.
- Selector de dispositivo de audio.
- Selector de entrada de audio.
- Selector de salida estéreo.
- Selector de buffer:
  - 64 samples
  - 128 samples
  - 256 samples
  - 512 samples
- Visualización de sample rate actual.
- Latencia aproximada.
- Medidor visual de entrada.
- Medidor visual de salida.
- Botón grande **Audio ON/OFF**.
- Botón grande **Mute / Panic**.
- Botón **Load VST3 Plugin**.
- Carga de un solo plugin VST3 a la vez.
- Modo bypass cuando no hay plugin cargado:

```text
Input → Output
```

- Modo plugin cuando hay VST3 cargado:

```text
Input → VST3 Plugin → Output
```

- Apertura/cierre del editor visual del plugin si el plugin lo permite.
- Mensajes claros si el plugin falla al cargar.
- Configuración local en JSON:
  - Último dispositivo.
  - Última entrada/salida.
  - Último buffer.
  - Última ruta de plugin cargado.
- Restauración de configuración al iniciar.
- Funcionamiento offline.

---

## Requisitos para compilar localmente en Windows

Necesitas tener instalado:

1. **Windows 10/11**.
2. **Visual Studio 2022** con carga de trabajo **Desktop development with C++**.
3. **CMake 3.24 o superior**.
4. **Git**.

No necesitas descargar JUCE manualmente para este proyecto: CMake lo descarga automáticamente usando `FetchContent`.

La versión fijada por defecto es:

```text
JUCE 8.0.13
```

Puedes cambiarla en `CMakeLists.txt` o pasando:

```powershell
-DMUSIVST_JUCE_TAG=8.0.13
```

---

## Cómo compilar localmente

Abre PowerShell en la carpeta del proyecto y ejecuta:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --target MusiVSTGuitarHost --parallel
```

El ejecutable quedará normalmente en alguna ruta parecida a:

```text
build/bin/Release/MusiVST Guitar Host.exe
```

o:

```text
build/MusiVSTGuitarHost_artefacts/Release/MusiVST Guitar Host.exe
```

CMake y Visual Studio, en su infinita poesía burocrática, pueden variar un poco la carpeta final.

---

## Cómo generar el ZIP portable con GitHub Actions

Este proyecto trae un workflow en:

```text
.github/workflows/windows-build.yml
```

Para usarlo:

1. Sube este proyecto a un repositorio de GitHub.
2. Entra a la pestaña **Actions**.
3. Ejecuta el workflow **Build Windows Portable ZIP**.
4. Al terminar, descarga el artifact:

```text
MusiVST-GuitarHost-Portable-Windows.zip
```

El ZIP contiene:

```text
MusiVST-GuitarHost-Portable-Windows/
├── MusiVST Guitar Host.exe
├── config/
├── assets/
└── README.txt
```

---

## Cómo conectar la guitarra

1. Conecta la guitarra eléctrica al **Input 1** de tu interfaz de audio.
2. Conecta la interfaz al computador.
3. Conecta tus parlantes, monitores o consola a las salidas principales de la interfaz, normalmente **Output 1-2**.
4. Abre **MusiVST Guitar Host**.
5. En el panel **Dispositivo de audio**, selecciona tu interfaz.
6. Activa Input 1.
7. Activa Output 1-2.
8. Presiona **Audio ON**.
9. Toca y revisa el medidor de entrada.

---

## Cómo cargar un plugin VST3

1. Presiona **Load VST3 Plugin**.
2. Busca un plugin `.vst3`.
3. En Windows suelen estar en:

```text
C:\Program Files\Common Files\VST3
```

4. Selecciona el archivo o carpeta `.vst3`.
5. Si carga bien, verás el nombre del plugin.
6. Presiona **Abrir editor** para ver su interfaz.

Importante: la app **no incluye plugins de guitarra**. El plugin VST3 debe estar instalado aparte o disponible en una ubicación que puedas seleccionar.

---

## Qué hacer si no suena

Revisa esto en orden:

1. Que la interfaz esté seleccionada como dispositivo de audio.
2. Que Input 1 esté activo.
3. Que Output 1-2 esté activo.
4. Que **Audio ON** esté activado.
5. Que **Mute / Panic** no esté en modo mute.
6. Que el medidor de entrada se mueva al tocar.
7. Que la ganancia de entrada de la interfaz no esté en cero.
8. Que el volumen de salida de la interfaz esté arriba.
9. Que el plugin no tenga su propio bypass o salida apagada.
10. Prueba sin plugin: si no hay plugin cargado, la app hace bypass directo.

Si el medidor de entrada se mueve pero el de salida no, el problema probablemente está en el plugin o en mute.

Si ningún medidor se mueve, el problema está antes de la app: cable, guitarra, interfaz, ganancia o entrada seleccionada.

---

## Qué hacer si hay mucha latencia

Prueba primero:

- Buffer **64** para menor latencia.
- Buffer **128** si 64 mete cortes.

Recomendación práctica:

```text
64 o 128 → tocar en vivo
256 o 512 → si hay cortes, clicks o CPU sufriendo como oficinista en cierre contable
```

También ayuda:

- Cerrar apps pesadas.
- Usar la interfaz como dispositivo principal dentro de la app.
- Evitar plugins muy pesados o con oversampling alto.
- Usar sample rate 44.1 kHz o 48 kHz para empezar.
- Revisar que el driver de la interfaz esté instalado.

Nota sobre ASIO: esta v0.1 usa los dispositivos que JUCE expone en Windows con la configuración actual del proyecto. Para latencias muy agresivas con algunas interfaces puede hacer falta compilar con soporte ASIO y tener el driver/SDK correspondiente disponible. No es obligatorio para probar la arquitectura inicial.

---

## Qué hacer si el plugin no carga

1. Confirma que sea VST3, no VST2, AU, AAX ni LV2.
2. Confirma que sea de 64 bits.
3. Confirma que esté instalado correctamente.
4. Prueba otro plugin VST3 sencillo.
5. Si el plugin requiere licenciamiento externo, abre primero su app/licenser.
6. Mira el mensaje de error mostrado por la app.

La app evita cerrarse si un plugin falla al cargar, pero un plugin defectuoso puede portarse como plugin defectuoso. Impactante, lo sé.

---

## Configuración local

La app crea o actualiza este archivo junto al `.exe`:

```text
config/user-settings.json
```

Ahí guarda la última configuración usada.

El archivo base es:

```text
config/default-settings.json
```

---

## Estructura del proyecto

```text
MusiVST-GuitarHost/
├── CMakeLists.txt
├── README.md
├── Source/
│   ├── Main.cpp
│   ├── MainComponent.h
│   ├── MainComponent.cpp
│   ├── AudioEngine.h
│   ├── AudioEngine.cpp
│   ├── PluginHostManager.h
│   ├── PluginHostManager.cpp
│   ├── SettingsManager.h
│   ├── SettingsManager.cpp
│   └── UI/
│       ├── LevelMeter.h
│       ├── LevelMeter.cpp
│       ├── DevicePanel.h
│       ├── DevicePanel.cpp
│       ├── PluginPanel.h
│       └── PluginPanel.cpp
├── assets/
│   └── logo.png
├── config/
│   └── default-settings.json
└── .github/
    └── workflows/
        └── windows-build.yml
```

---

## Decisiones técnicas importantes

- El audio thread no carga plugins.
- El audio thread no lee ni escribe archivos.
- El audio thread no hace operaciones JSON.
- Si el plugin está siendo cambiado, la app prefiere silenciar antes que meter ruido.
- Si no hay plugin, se usa bypass directo.
- La app usa un solo plugin a la vez.
- El motor trabaja internamente en estéreo.
- Input 1 se duplica a estéreo para plugins típicos de guitarra.
- Si hay Input 2 disponible, se usa como canal derecho.

---

## Licencia de JUCE

JUCE tiene sus propias condiciones de licencia. Antes de distribuir públicamente una app cerrada o comercial, revisa la licencia vigente de JUCE y decide si usarás modalidad open source compatible o licencia comercial.
