# C++ HTTP Server

Servidor HTTP escrito em C++ com:
- sockets
- thread pool
- HTTP parsing

# Build

```bash
mkdir build
cd build
cmake ..
make
```
Para testar o servidor:
```bash
./server --help
./server-cpp --p 8080 --thread 5 --main-html ../example_archives/index.html --files ../example_archives/script.js ../example_archives/style.css
```
## Como instalar o programa
```bash
sudo cp server-cpp /usr/bin
```
Assim podendo rodar o codigo a partir de qualquer diretorio.
