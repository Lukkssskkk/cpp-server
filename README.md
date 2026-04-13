# C++ HTTP Server
Servidor HTTP escrito em C++ com:
- sockets
- thread pool
- HTTP parsing
- Filesystems
# Necessidades do codigo
- Cmake
- Socket
- C++ >= C++20
- make
# Instalação das necessidades:
Distros baseadas em Debian,Ubunto:
```build
sudo apt install build-essential
```
Distros baseadas em Arch:
```build
sudo pacman -S cmake make g++ gcc
```
Distros baseadas em Gentoo:
```build
sudo emerge cmake
```
# Build
Compilação do servidor C++ via cmake:
```bash
mkdir build
cd build
cmake ..
make
```
Para testar o servidor:
```bash
./http-server-cpp --help
./http-server-cpp --p 8080 --thread 5 --root ../example_archives --main-html index.html
```
## Como instalar o programa
```bash
sudo cp server-cpp /usr/bin
```
Assim podendo rodar o codigo a partir de qualquer diretorio.
