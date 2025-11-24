# Instrucciones para MAC

1. Abre una terminal

2. Instalar HomeBrew. Pega lo siguiente en una terminal.

```
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```
3. Instala Git. Pega lo siguiente en una terminal

```
brew install git
```

4. Sigue la instrucciones de [la página de espressif](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html).

```
mkdir -p ~/Documents/Arduino/hardware/espressif && \
cd ~/Documents/Arduino/hardware/espressif && \
git clone https://github.com/espressif/arduino-esp32.git esp32 && \
cd esp32/tools && \
python get.py
```

