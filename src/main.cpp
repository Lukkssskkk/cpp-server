//Bibliotecas C-like para coisas basicas de rede e C
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
//Bibliotecas da std uteis
#include<iostream>
#include<fstream>
#include<vector>
#include<string>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
//Multitheads
std::queue<int> socket_queue;
std::mutex queue_mutex;
std::condition_variable queue_cv;
std::mutex log_mutex;

//Variaveis Globais
struct sockaddr_in address;
int addrlen = sizeof(address);
std::fstream logg("server.log",std::ios::trunc | std::ios::out);

void failure(std::string ERROR_MESSAGE,int ERROR_TYPE){
    std::lock_guard<std::mutex> lock(log_mutex);

    if(ERROR_TYPE==0){
        logg<<ERROR_MESSAGE;
        std::cout<<ERROR_MESSAGE;
        logg.close();
        exit(EXIT_FAILURE);
    }else if(ERROR_TYPE==1){
        logg<<"[ERRO DE IMPORTACAO]"<<ERROR_MESSAGE;
        std::cout<<ERROR_MESSAGE;
    }
}

std::string define_type(const std::string& def_type){
    if(def_type.find(".html")!= std::string::npos){
        return "text/html";
    }else if(def_type.find(".css")!=std::string::npos){
        return "text/css";
    }else if(def_type.find(".js")!=std::string::npos){
        return "application/javascript";
    }
    return "text/plain";
}

std::string get_file(const std::string& file_path){
    std::ifstream file(file_path, std::ios::binary);
    if(!file){
        failure("Cannot open the request file: "+file_path+"\n",1);
        return "";
    }
    return std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>()
    );
}

int init_socket(int port){
    int server_fd;
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        failure("Cannot create the requisited socket\n",0);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        failure("Cannot reserve the adress to the socket\n",0);
    }
    if (listen(server_fd, 128) < 0) {
        failure("Cannot listen the server\n",0);
    }
    printf("Servidor HTTP iniciado na porta %d...\n", port);
    return server_fd;
}

std::string response(std::string archive,int size,std::string type){
    if(!archive.empty()){
        std::string resp;
        resp="HTTP/1.1 200 OK\r\nContent-Type: ";
        resp+=type;
        resp+="\r\nContent-Length: ";
        resp+=std::to_string(size);
        resp+="\r\n\r\n";
        resp+=archive;
        return resp;
    }else{
        std::string msg = "404 Not Found";
        return "HTTP/1.1 404 Not Found\r\nContent-Length: " +
        std::to_string(msg.size()) + "\r\n\r\n" + msg;
    }
}

std::string server_read(int socket) {
    std::string request;
    char buffer[2048];
    int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        buffer[bytes] = '\0';
        request.append(buffer, bytes);
    }

    {
        std::lock_guard<std::mutex> lock(log_mutex);
        if(!request.empty()) {
            std::cout << "REQUEST RECEBIDA" << '\n';
        }
    }

    return request;
}


std::string get_path(const std::string& request){
    size_t start = request.find("GET ");
    size_t end   = request.find(" HTTP/1.1");

    if(start == std::string::npos || end == std::string::npos)
        return "/";

    start += 4;
    return request.substr(start, end - start);
}

void cliente(std::string main_html){
    while(true){
        int new_socket;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);

            queue_cv.wait(lock, []{
                return !socket_queue.empty();
            });

            new_socket = socket_queue.front();
            socket_queue.pop();
        }

        std::string request = server_read(new_socket);
        if(request.empty()){
            close(new_socket);
            continue;
        }

        std::string path = get_path(request);

        if (path.find("..") != std::string::npos) {
            std::string msg = "403 Forbidden";
            std::string resp = "HTTP/1.1 403 Forbidden\r\nContent-Length: " +
                std::to_string(msg.size()) + "\r\n\r\n" + msg;

            write(new_socket, resp.c_str(), resp.size());
            close(new_socket);
            continue;
        }

        std::string file_path;
        if(path == "/") file_path = main_html;
        else file_path = "." + path;

        std::string content = get_file(file_path);

        std::string resp = response(content, content.size(), define_type(file_path));

        write(new_socket, resp.c_str(), resp.size());
        close(new_socket);
    }
}

int main(int argc,char** argv){

    int PORT=8080,Nthreads=1;
    std::vector<std::string> files_import;
    std::string main_html="index.html";

    for(int i=0;i<argc;i++){
        std::string temp=argv[i];

        if(temp=="--p"){
            if(i+1<argc){
                PORT=std::stoi(argv[i+1]);
                i++;
            }
        }
        else if(temp=="--thread"){
            if(i+1<argc){
                Nthreads=std::stoi(argv[i+1]);
                i++;
            }
        }
        else if(temp=="--main-html"){
            if(i+1<argc){
                main_html=argv[i+1];
                i++;
            }
        }
        else if(temp=="--help"){ 
            std::cout<<"Comando de criação de servidor http em C++\n"<< "--help\t\t\t Ajuda;\n"
                << "--p\t\t\t Porta;\n"
                << "--thread\t\t Threads utilizados;\n"
                << "--main-html\t\t Arquivo principal;\n"
                << "--files [...]\t\t Arquivos extras;\n"
                <<"Exemplo de uso: server-cpp --p 8080 --thread 12 --main-html index.html --files script.js style.css\n";
            exit(EXIT_SUCCESS);
        }
    }
    if(argc==0){
        std::cout<<"ERRO: Nenhum arquivo ou porta citado\n";
        std::cout<<"Comando de criação de servidor http em C++\n"<< "--help\t\t\t Ajuda;\n"
            << "--p\t\t\t Porta;\n"
            << "--thread\t\t Threads utilizados;\n"
            << "--main-html\t\t Arquivo principal;\n"
            << "--files [...]\t\t Arquivos extras;\n"
            <<"Exemplo de uso: server-cpp --p 8080 --thread 12 --main-html index.html --files script.js style.css\n";
        exit(EXIT_SUCCESS);
    }

    if(Nthreads<=0){
        failure("Numero de threads deve ser maior que 0\n",0);
    }
    int server_fd = init_socket(PORT);

    std::vector<std::thread> workers;
    for(int i = 0; i < Nthreads; i++){
        workers.emplace_back(cliente, main_html);
    }

    while (1) {
        int new_socket;

        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
            failure("Não foi possivel aceitar requisição\n",1);
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            socket_queue.push(new_socket);
        }

        queue_cv.notify_one();
    }

    logg.close();
    close(server_fd);

    return 0;
}