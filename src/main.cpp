//Bibliotecas C-like
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
//Std
#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include<filesystem>
//Multithreads
std::queue<int> socket_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;
std::mutex log_mutex;
//Filesystem
namespace fs = std::filesystem;
//Globais
struct sockaddr_in address;
int addrlen = sizeof(address);
std::ofstream logg("server.log");

std::string Error_on_site(std::string type){
    std::string msg = type;
    return "HTTP/1.1 " + type + "\r\n"
           "Content-Type: text/plain\r\n"
           "Content-Length: " + std::to_string(msg.size()) + "\r\n\r\n" +
           msg;
}

void failure(std::string ERROR_MESSAGE,int ERROR_TYPE){
    std::lock_guard<std::mutex> lock(log_mutex);

    if(ERROR_TYPE==0){
        logg<<ERROR_MESSAGE;
        std::cout<<ERROR_MESSAGE;
        logg.close();
        exit(EXIT_FAILURE);
    }else{
        logg<<"[ERRO] "<<ERROR_MESSAGE;
        std::cout<<ERROR_MESSAGE;
    }
}

std::string define_type(const std::string& path){
    std::string ext = fs::path(path).extension().string();
    //Arquivos de configuração e codigo do site
    if(ext == ".html") return "text/html";
    if(ext == ".css")  return "text/css";
    if(ext == ".js")   return "application/javascript";
    if(ext == ".wasm") return "application/wasm";
    //Extras
    if(ext == ".png")  return "image/png";
    if(ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if(ext == ".gif")  return "image/gif";

    return "application/octet-stream";
}

std::string get_file(const std::string& file_path){
    std::ifstream file(file_path, std::ios::binary);
    if(!file){
        failure("Cannot open: "+file_path+"\n",1);
        return "";
    }

    return std::string((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
}

int init_socket(int port){
    int server_fd;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        failure("Socket error\n",0);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        failure("Bind error\n",0);
    }
    if (listen(server_fd, 128) < 0){
        failure("Listen error\n",0);
    }
    printf("Servidor HTTP na porta %d\n", port);
    return server_fd;
}

std::string response(std::string archive,int size,std::string type){
    if(!archive.empty()){
        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: " + type + "\r\n"
               "Content-Length: " + std::to_string(size) + "\r\n\r\n" +
               archive;
    }else{
        return Error_on_site("404 Not Found");
    }
}

std::string server_read(int socket) {
    std::string request;
    char buffer[2048];
    int bytes = recv(socket, buffer, sizeof(buffer)-1, 0);

    if (bytes > 0){
        buffer[bytes] = '\0';
        request.append(buffer, bytes);
    }
    if(!request.empty()){
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "REQUEST RECEBIDA:\n" << request << "\n";
    }

    return request;
}

std::string get_path(const std::string& request){
    size_t start = request.find("GET ");
    size_t end   = request.find(" HTTP/1.1");

    if(start == std::string::npos || end == std::string::npos){
        return "/";
    }
    return request.substr(start+4, end - (start+4));
}

void cliente(std::string main_html, std::string root_dir){
    fs::path root;

    try{
        root = fs::canonical(fs::path(root_dir));
    }catch(...){
        failure("Root directory inválido\n",0);
    }

    while(true){
        int new_socket;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, []{ return !socket_queue.empty(); });

            new_socket = socket_queue.front();
            socket_queue.pop();
        }

        std::string request = server_read(new_socket);
        if(request.empty()){
            close(new_socket);
            continue;
        }

        std::string path = get_path(request);
        fs::path requested;

        if(path == "/"){
            requested = root / main_html;
        }
        else{
            requested = root / path.substr(1);
        }
        requested = fs::weakly_canonical(requested);

        if (requested.native().compare(0, root.native().size(), root.native()) != 0){
            std::string resp = Error_on_site("403 Forbidden");
            write(new_socket, resp.c_str(), resp.size());
            close(new_socket);
            continue;
        }
        if (!fs::exists(requested) || !fs::is_regular_file(requested)){
            std::string resp = Error_on_site("404 Not Found");
            write(new_socket, resp.c_str(), resp.size());
            close(new_socket);
            continue;
        }

        std::string content = get_file(requested.string());
        std::string resp = response(content, content.size(), define_type(requested.string()));

        write(new_socket, resp.c_str(), resp.size());
        close(new_socket);
    }
}

int main(int argc,char** argv){

    int PORT=8080, Nthreads=1;
    std::string root_dir=".";
    std::string main_html="index.html";

    for(int i=1;i<argc;i++){
        std::string temp=argv[i];

        if(temp=="--p" && i+1<argc){
            PORT=std::stoi(argv[++i]);
        }
        else if(temp=="--thread" && i+1<argc){
            Nthreads=std::stoi(argv[++i]);
        }
        else if(temp=="--main-html" && i+1<argc){
            main_html=argv[++i];
        }
        else if(temp=="--root" && i+1<argc){
            root_dir=argv[++i];
        }
        else if(temp=="--help"){
            std::cout<<"Comando de criação de servidor http em C++\n" 
            << "--help\t\t\t\t\t Ajuda;\n" 
            << "--p [NUMERO]\t\t\t\t Porta;\n" 
            << "--thread [NUMERO]\t\t\t Threads utilizados;\n" 
            << "--root [PASTA]\t\t\t\t Pasta raiz;\n" 
            << "--main-html [Central do site]\t\t Arquivo principal;\n" 
            <<"USO: http-server-cpp --p 8080 --thread 12 --root ./public --main-html index.html\n"; 
            exit(EXIT_SUCCESS);
        }
        else{
            failure("Argumento inválido: " + temp + "\n",0);
        }
    }

    if(argc == 1){
        failure("Argumentos insuficientes\n",0);
    }
    if(Nthreads <= 0){
        failure("Threads invalid\n",0);
    }

    int server_fd = init_socket(PORT);
    std::vector<std::thread> workers;

    for(int i = 0; i < Nthreads; i++){
        workers.emplace_back(cliente, main_html, root_dir);
    }

    while (1){
        int new_socket;

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0){
            failure("Accept error\n",1);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            socket_queue.push(new_socket);
        }
        queue_cv.notify_one();
    }

    close(server_fd);
    logg.close();
    return 0;
}
