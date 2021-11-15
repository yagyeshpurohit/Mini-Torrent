#include<bits/stdc++.h>
#include<netdb.h>
#include<unistd.h>
#include<pthread.h>
#include<arpa/inet.h>
#include<dirent.h>
#include<sys/stat.h>
#include<stdio.h>

#define SERVER_PORT "45000"
#define MAXDATASIZE 100
#define BACKLOG 10

#define CHUNK_SIZE 512 * 1024

#define ESC '\x1b'

#define rep(i,a,b) for(int i=a;i<b;i++)
#define erep(i,a,b) for(int i=a;i<=b;i++)

using namespace std;

// ----------------------------------- Constant int

// IMP
const int EXIT = 1;
const int REGISTER = 2;
const int LOG_IN = 3;
const int LOG_OUT = 4;
const int CREATE_GROUP = 5;
const int LIST_GROUP = 6;
const int JOIN_GROUP = 7;
const int LIST_GROUP_JOIN_REQ = 8;
const int REPLY_JOIN_REQ = 9;
const int UPLOAD_FILE = 10;
const int LIST_FILE = 11;
const int DOWNLOAD_FILE = 12;
const int SHOW_DOWNLOAD = 13;
const int LEAVE_GROUP = 14;

const int DO_NOTHING = 9999;
    
// extra
const int SHOW_ALL_INFO = 9998;
const int SEND_MSG_TO_PEER = 9997;

// ----------------------------------- Some globle var
class file_info_class{
    public:
        int id;

        string name;
        int size;

        int bitmap_len;
        vector<bool> bitmap;
};

file_info_class * getNewFile(int id, string name, int size){
    file_info_class * new_file = new file_info_class;

    new_file->id = id;
    new_file->name = name;
    new_file->size = size;

    int bitmap_len = size/(512 * 1024);
    new_file->bitmap_len = bitmap_len;

    new_file->bitmap = vector<bool>(bitmap_len, true);

    return new_file;
}

class globle_info_class{

    public:
        bool login_status;
        int curr_log_in_id;

        string user_name;

        pthread_t serv_thread;
        vector<pthread_t> serv_join;

        map<int, file_info_class *> files;
        int sockfd;

        string curr_path;

        globle_info_class(){
            login_status = false;
        }

        void addFile(int file_id, file_info_class * file){
            files.insert({file_id, file});
        }
};

globle_info_class * globle_info = new globle_info_class();

// ----------------------------------- Class

// 
class usr_info_class{
    public: 
        int id;
        bool login_status;

        vector<int> groups;

        void addGroup(int grp_id){
            groups.push_back(grp_id);
        }

        void printGroup(){
            cout<<endl<<"Group ["<<groups.size()<<"]: \n";
            for(int j=0; j<groups.size(); j++){
                cout<<groups[j]<<" ";
            }
            cout<<endl;
        }
};

usr_info_class * getNewUsrInfo(int peer_id){
    usr_info_class * my_info = new usr_info_class;
    my_info->id = peer_id;

    return  my_info;
}

map<string, usr_info_class *> local_users;

// ----------------------------------- Structure & it's method incoming connection information 

// --------- Self Information
// structure
class self_server_info_struct{
    public:
        char * port;
        char * ip;
};

// variable of struct
self_server_info_struct * self_server_info = new self_server_info_struct;


// --------- incoming connection information
// Structure
typedef struct info_structure{
    int sockfd;
    int newfd;
    struct sockaddr_storage their_addr;
    socklen_t their_addr_len;
}info_struct;

// It's Method
info_struct * makeInfoStruct(int sockfd, int newfd, struct sockaddr_storage their_addr, socklen_t their_addr_len){

    info_struct * new_struct = (info_struct *)malloc(sizeof(info_struct));

    new_struct->sockfd = sockfd;
    new_struct->newfd = newfd;

    new_struct->their_addr = their_addr;
    new_struct->their_addr_len = their_addr_len;

    return new_struct;
}



// ----------------------------------- Methods 


// --------- Sender Address fetch 
// addr: from Unspecific to Ipv4 or Ipv6
void *getInAddr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// get ip and port from addr storage
void getIpPort(info_struct * info, string & recver_ip, int & recver_port){
    char input_ip[INET6_ADDRSTRLEN];
    
    inet_ntop(info->their_addr.ss_family, getInAddr((struct sockaddr *)&(info->their_addr)), input_ip, sizeof(input_ip));
    recver_ip = input_ip;
 
    recver_port = ntohs(((struct sockaddr_in*)(struct sockaddr *)&(info->their_addr))->sin_port);
}


// --------- Peer Menu Function
string getPeerMenu(){
    string peer_menu = "";
    
    peer_menu.append("1.\tEXIT\n"); 
    peer_menu.append("2.\tREGISTER\n");
    peer_menu.append("3.\tLOG IN\n");
    peer_menu.append("4.\tLOG OUT\n");
    peer_menu.append("5.\tCREATE GROUP\n");
    peer_menu.append("6.\tLIST GROUP\n");
    peer_menu.append("7.\tJOIN GROUP\n");
    peer_menu.append("8.\tLIST GROUP JOIN REQUEST\n");
    peer_menu.append("9.\tREPLY JOIN REQUEST\n");
    peer_menu.append("10.\tUPLOAD FILE\n");
    peer_menu.append("11.\tLIST FILE\n");
    peer_menu.append("12.\tDOWNLOAD FILE\n");
    peer_menu.append("9998.\tAll user info on server\n");

    return peer_menu;
}

// --------- Send Recv Msges
// buffer append fun
void bufferAppendToMsg(char *buffer, int s_ind, int e_ind, string & msg){
    erep(i,s_ind,e_ind){
        msg.push_back(buffer[i]);
    }
}

// receive fun
bool recvMsg(int sockfd, string & recv_msg){
    recv_msg.clear();

    char buffer[MAXDATASIZE];

    int byte_count = recv(sockfd, buffer, MAXDATASIZE-1, 0);
    if(byte_count < 1){
        perror("RECV");
        close(sockfd);
        // exit(1);

        return false;
    }

    int msg_len;
    int remaing_byte;

    string msg_len_str;
    
    int header_len = 0;
    rep(i,0,byte_count){
        header_len++;
        if(buffer[i] == ESC){
            break;
        }else{
            msg_len_str.push_back(buffer[i]);
        }
    }

    // cout<<"Byte count: "<<byte_count<<endl;
    // cout<<"msg_len_str: "<<msg_len_str<<endl;

    msg_len = stoi(msg_len_str);
    remaing_byte = msg_len - byte_count + header_len;

    bufferAppendToMsg(buffer, header_len, byte_count-1, recv_msg);

    while(remaing_byte){
        int byte_count = recv(sockfd, buffer, MAXDATASIZE-1, 0);
        if(byte_count < 1){
            perror("RECV");
            close(sockfd);
            // exit(1);

            return false;
        }

        bufferAppendToMsg(buffer, 0, byte_count-1, recv_msg);
    
        remaing_byte -= byte_count;
    }

    return true;
}

// Send Fun
bool sendMsg(int sockfd, string send_msg){

    int return_val;

    int msg_len = send_msg.length();
    string header = to_string(msg_len) + ESC;

    string msg_chunk;
    string msg = header + send_msg;

    int loop_counter = ceil((float)msg.length()/MAXDATASIZE);

    rep(i,0,loop_counter){
        msg_chunk = msg.substr( (i*MAXDATASIZE), MAXDATASIZE);

        return_val = send(sockfd, msg_chunk.c_str(), msg_chunk.length(), 0);
        if(return_val == -1){
            perror("SEND");
            close(sockfd);
            // exit(1);

            return false;
        }
    }

    return true;
}


// --------- Reply msg deciding function
// Header
bool makeHeader(int choice, string & header){

    int header_len = 4;
    char buffer[8];

    snprintf( buffer, sizeof(buffer), "%.*d", header_len, choice );
    header = buffer;

    return true;
}

// msg vector
int getMsgVector(string & msg, vector<string> & msg_vector){
    int msg_len = msg.length();
    string msg_chunck;
    
    int vector_len = 0;

    for(int i=0; i<msg_len; i++){
        if(msg[i] == ESC){
            if(msg_chunck.length() > 0){
                msg_vector.push_back(msg_chunck);
                msg_chunck.clear();
                vector_len++;
            }
        }else{
            msg_chunck.push_back(msg[i]);
        }
    }

    return vector_len;
}

int stringToVector(string & msg, vector<string> & msg_vector, char delimeter){
    
    msg_vector.clear();

    int msg_len = msg.length();
    string msg_chunck;
    
    int vector_len = 0;

    for(int i=0; i<msg_len; i++){
        if(msg[i] == delimeter){
            if(msg_chunck.length() > 0){
                msg_vector.push_back(msg_chunck);
                msg_chunck.clear();
                vector_len++;
            }
        }else{
            msg_chunck.push_back(msg[i]);
        }
    }

    if(msg_chunck.length() > 0){
        msg_vector.push_back(msg_chunck);
    }

    return vector_len;
}

// ----------------------------------- Peer Server methods
// reply msg
bool getReplyMsg(string & recv_msg, string & reply_msg, info_struct * info){
    reply_msg.clear();

    string recver_ip;
    int recver_port;

    getIpPort(info, recver_ip, recver_port);

    vector<string> recv_msg_vector;
    int recv_vector_len = getMsgVector(recv_msg, recv_msg_vector);
                    
    int file_id = stoi(recv_msg_vector[0]);
    string file_path = recv_msg_vector[1];

    int seekpoint = stoi(recv_msg_vector[2]);
    int data_len = stoi(recv_msg_vector[3]);

    // string file_path = globle_info->curr_path + "/" + file_name;

    std::ifstream infile (file_path.c_str(),std::ifstream::binary);

    // // allocate memory for file content
    char * buffer = new char[data_len+1];

    // // read content of infile
    infile.seekg (seekpoint,infile.beg);
    infile.read (buffer,data_len);


    // cout<<"peerClient <"<<seekpoint<<">"<<endl;

    // FILE  * in_ptr = fopen(file_path.c_str(), "r");

    // fseek(in_ptr, seekpoint, SEEK_SET);
    // fgets(buffer, data_len+1, in_ptr);

    // fclose(in_ptr);

    reply_msg = buffer;

    // release dynamically-allocated memory
    delete[] buffer;

    // outfile.close();
    infile.close();

    return true;
}

// --------- Peer - Peer conversion
void * peerPeerConversationFun(void *input){
    
    info_struct * info = (info_struct *)input;

    string recver_ip;
    int recver_port;

    getIpPort(info, recver_ip, recver_port);

    // cout<<"<"<<recver_ip<<" , "<<recver_port<<"> :: "<<"Conversation start"<<endl;

    string recv_msg;
    string reply_msg;

    bool flag = true;

    while(flag){

        flag = recvMsg(info->newfd, recv_msg);
        // cout<<"<"<<recver_ip<<" , "<<recver_port<<"> :: "<<recv_msg<<endl;

        if(flag){

            flag = getReplyMsg(recv_msg, reply_msg, info);

            if(flag){
                flag = sendMsg(info->newfd, reply_msg);
            }
        }       
    }

    // cout<<"<"<<recver_ip<<" , "<<recver_port<<"> :: "<<"Conversion Complete"<<endl;

    close(info->newfd);    
    pthread_exit(NULL);
}

// --------- Peer Server thread function
void * peerServer(void * arg){

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);
    // cout<<endl<<endl<<"SERVER :: Peer-Server Running"<<endl;

    struct addrinfo hints;
    struct addrinfo *servinfo, *rev;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags = AI_PASSIVE;

    int return_val;
    // cout<<self_server_info->ip<<":"<<self_server_info->port;
    return_val = getaddrinfo(self_server_info->ip, self_server_info->port, &hints, &servinfo);

    if(return_val != 0){
        perror(gai_strerror(return_val));
        return NULL;
    }

    int sockfd; // Socket File Descriptor
    int yes = 1;

    for(rev = servinfo; rev != NULL; rev = rev->ai_next){

        sockfd = socket(rev->ai_family, rev->ai_socktype, rev->ai_protocol);
        if(sockfd == -1){
            perror("SOCKET");
            continue;
        }

        globle_info->sockfd = sockfd;

        return_val = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
        if(return_val == -1){
            perror("setsockopt");
            exit(1);
        }

        return_val = bind(sockfd, rev->ai_addr, rev->ai_addrlen);
        if(return_val == -1){
            close(sockfd);
            perror("BIND");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if(rev == NULL){
        perror("NO MATCH: BIND");
        exit(1);
    }

    return_val = listen(sockfd, BACKLOG);
    if(return_val == -1){
        perror("LISTEN");
        exit(1);
    }

    int thread_counter= 0;

    while(true){

        thread_counter++;
        // cout<<endl<<endl<<"SERVER :: thread_counter: "<<thread_counter<<endl;

        pthread_t new_thread;

        struct sockaddr_storage their_addr;
        socklen_t their_addr_len = sizeof(their_addr);

        int newfd; // New Socket Descriptor

        newfd = accept( sockfd, (struct sockaddr *)&their_addr, &their_addr_len);
        if(newfd == -1){
            perror("ACCEPT");
            continue;
        }

        info_struct *info = makeInfoStruct(sockfd, newfd, their_addr, their_addr_len);

        return_val = pthread_create(&new_thread, NULL, &peerPeerConversationFun, (void *)info);
        if(return_val){
            perror("THREAD");
            close(newfd);
            close(sockfd);
            exit(1);
        }
    }

    close(sockfd);
    pthread_exit(NULL);
}

// grt choice
int getChoice(string & choice_str){

    int len = choice_str.length();
    int choice = 0;

    char ch;

    for(int i=0;i<len;i++){

        ch = choice_str[i];

        if( ch >= '0' && ch <= '9'){
            choice = 10*choice + (ch - '0');
        }else{
            return -1;
        }
    }

    return choice;
}

// -------- Conversation as peer methods


void stringToCharArr(string input, char ** output){
    int in_str_len = input.length();

    *output = new char[in_str_len+1];
    strcpy(*output, input.c_str());
}

void getLocalFileInfo(vector<vector<string>> & curr_path_content_info){

    vector<string> curr_path_content;

    char path[4096];
    string path_str = path;

    getcwd(path, sizeof(path));

    path_str = path;
    path_str.push_back('/');
    path_str.append("peer");
    path_str.append(to_string(globle_info->curr_log_in_id));
    // path_str.push_back('/');

    globle_info->curr_path = path_str;

    struct dirent *dir_entry_ptr;
    DIR *dir_ptr = opendir(path_str.c_str());

    while ((dir_entry_ptr = readdir(dir_ptr)))
    {
        curr_path_content.push_back(dir_entry_ptr->d_name);
    }    
    closedir(dir_ptr);

    for(auto i=curr_path_content.begin(); i!= curr_path_content.end(); i++){

        string content_path_str = path_str;
        content_path_str.append("/" + (*i));
        
        char * content_path_char;

        stringToCharArr(content_path_str, &content_path_char);

        struct stat file_info;
        stat(content_path_char, &file_info);

        vector<string> curr_content_info;

        // Adding elements to --> curr_content_info

        // --------------------------------------------------------PERMISSION
        bool is_file;

        is_file = S_ISDIR(file_info.st_mode) ? false : true ;
            
        if(is_file){
            
            // ------------- Adding current content info    

            // --------------------------------------------------------NAME
            curr_content_info.push_back(*i);


            // --------------------------------------------------------SIZE
            string size_str = to_string(file_info.st_size);
            curr_content_info.push_back(size_str);

            // // --------------------------------------------------------OWNERSHIP

            curr_path_content_info.emplace_back(curr_content_info);
        }
    }
}

// send msg
bool getSendMsg(int choice, string & send_msg, vector<string> & cmd_input){
    send_msg.clear();

    string header;
    makeHeader(choice, header);

    usr_info_class * usr = NULL;

    string input;

    string user_name;
    string password;

    string do_nothing_hrader = "9999";

    string grp_id_str;
    int grp_id;

    string peer_id_str;
    int peer_id;

    string file_no_str;
    int file_no;

    send_msg = header;
    send_msg.push_back(ESC);

    vector<string> content_info;
    vector<vector<string>> curr_path_content_info;

    int cmd_input_size = cmd_input.size();

    switch(choice){

        case EXIT:
            return false;

        case REGISTER:

            if(!globle_info->login_status){
                // cout<<"\nEnter User-name: ";
                // cin>>user_name;

                // cout<<"Enter Password: ";
                // cin>>password;

                if(cmd_input_size >= 3){
                    user_name = cmd_input[1];
                    password = cmd_input[2];

                    send_msg.append(user_name);
                    send_msg.push_back(ESC);
                    
                    send_msg.append(password);
                    send_msg.push_back(ESC);

                    send_msg.append(self_server_info->ip);
                    send_msg.push_back(ESC);

                    send_msg.append(self_server_info->port);
                    send_msg.push_back(ESC);
                }else{
                    send_msg = do_nothing_hrader;
                    send_msg.push_back(ESC);
                }                
            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            }
            break;

        case LOG_IN:

            if(!globle_info->login_status){
                // cout<<"\nEnter User-name: ";
                // cin>>user_name;

                // cout<<"Enter Password: ";
                // cin>>password;

                if(cmd_input_size >= 3){
                    user_name = cmd_input[1];
                    password = cmd_input[2];

                    usr = local_users[user_name];

                    if(usr){
                        send_msg.append(to_string(usr->id));
                        send_msg.push_back(ESC);

                        send_msg.append(user_name);
                        send_msg.push_back(ESC);
                        
                        send_msg.append(password);
                        send_msg.push_back(ESC);
                    }else{
                        send_msg = do_nothing_hrader;
                        send_msg.push_back(ESC);
                    }
                }else{
                    send_msg = do_nothing_hrader;
                    send_msg.push_back(ESC);
                } 

            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            }            

            break;

        case LOG_OUT:

            if(globle_info->login_status){
                send_msg.append(to_string(globle_info->curr_log_in_id));
                send_msg.push_back(ESC);
            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            }   

            break;

        case CREATE_GROUP:

            if(globle_info->login_status){

                if(cmd_input_size >= 1){
                    grp_id = getChoice(cmd_input[1]);

                    if(grp_id == -1){
                        send_msg = do_nothing_hrader;
                        send_msg.push_back(ESC);

                    }else{

                        send_msg.append(to_string(globle_info->curr_log_in_id));
                        send_msg.push_back(ESC);

                        send_msg.append(to_string(grp_id));
                        send_msg.push_back(ESC);
                    }

                }else{
                    send_msg = do_nothing_hrader;
                    send_msg.push_back(ESC);

                }
            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            } 

            break;

        case LIST_GROUP:    
            break;

        case JOIN_GROUP:

            if(globle_info->login_status){
                // cout<<"\nEnter Group_id: ";
                // cin>>grp_id_str;

                if(cmd_input_size >= 1){
                    grp_id_str = cmd_input[1];
                    grp_id = getChoice(grp_id_str);

                    if(grp_id == -1){
                        send_msg = do_nothing_hrader;
                        send_msg.push_back(ESC);
                    }else{
                        send_msg.append(to_string(globle_info->curr_log_in_id));
                        send_msg.push_back(ESC);

                        send_msg.append(to_string(grp_id));
                        send_msg.push_back(ESC);
                    }
                }else{
                    send_msg = do_nothing_hrader;
                    send_msg.push_back(ESC);
                }              
            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            }
            break;

        case LIST_GROUP_JOIN_REQ:

            if(globle_info->login_status){
                send_msg.append(to_string(globle_info->curr_log_in_id));
                send_msg.push_back(ESC);
            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            }
            break;

        case REPLY_JOIN_REQ:
            if(globle_info->login_status){

                // cout<<"\nEnter Group_id: ";
                // cin>>grp_id_str;

                if(cmd_input_size >= 2){
                    grp_id_str = cmd_input[1];

                    grp_id = getChoice(grp_id_str);
                    if(grp_id == -1){
                        send_msg = do_nothing_hrader;
                        send_msg.push_back(ESC);
                        break;
                    }

                    // cout<<"\nEnter Peer_id: ";
                    // cin>>peer_id_str;

                    peer_id_str = cmd_input[2];

                    peer_id = getChoice(peer_id_str);
                    if(peer_id == -1){
                        send_msg = do_nothing_hrader;
                        send_msg.push_back(ESC);
                        break;
                    }


                    send_msg.append(to_string(grp_id));
                    send_msg.push_back(ESC);

                    send_msg.append(to_string(peer_id));
                    send_msg.push_back(ESC);        
                
                }else{
                    send_msg = do_nothing_hrader;
                    send_msg.push_back(ESC);
                }        
                
            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            }
            break;

        case UPLOAD_FILE:

            if(globle_info->login_status){

                // getLocalFileInfo(curr_path_content_info);

                // cout<<"\nCurrent Directory Files:\n";
                
                // cout<<setw(5)<<"No.";
                // cout<<setw(50)<<left<<"NAME";
                // cout<<"SIZE"<<endl;
                
                // for(int j=0; j<curr_path_content_info.size(); j++){

                //     content_info = curr_path_content_info[j];

                //     cout<<setw(5)<<j+1<<".";
                //     cout<<setw(50)<<left<<content_info[0]<<" ";
                //     cout<<content_info[1]<<endl;
                // }

                // cout<<"\nEnter File_No: ";
                // cin>>file_no_str;

                // file_no = getChoice(file_no_str);

                // if(file_no <= 0 || file_no > curr_path_content_info.size()){

                //     send_msg = do_nothing_hrader;
                //     send_msg.push_back(ESC);
                // }else{
                //     send_msg.append(to_string(globle_info->curr_log_in_id));
                //     send_msg.push_back(ESC);

                //     send_msg.append(curr_path_content_info[file_no-1][0]);
                //     send_msg.push_back(ESC);

                //     send_msg.append(curr_path_content_info[file_no-1][1]);
                //     send_msg.push_back(ESC);
                // }

                if(cmd_input_size >= 2){
                    struct stat file_info;
                    stat(cmd_input[1].c_str(), &file_info);

                    vector<string> curr_content_info;

                    

                    // Adding elements to --> curr_content_info

                    // --------------------------------------------------------PERMISSION
                    bool is_file;
                    is_file = S_ISDIR(file_info.st_mode) ? false : true ;
                        
                    if(is_file){
                        
                        // ------------- Adding current content info    

                        // --------------------------------------------------------NAME
                        curr_content_info.push_back(cmd_input[1]);


                        // --------------------------------------------------------SIZE
                        string size_str = to_string(file_info.st_size);
                        curr_content_info.push_back(size_str);

                        // // --------------------------------------------------------OWNERSHIP

                        send_msg.append(to_string(globle_info->curr_log_in_id));
                        send_msg.push_back(ESC);

                        send_msg.append(curr_content_info[0]);
                        send_msg.push_back(ESC);

                        send_msg.append(curr_content_info[1]);
                        send_msg.push_back(ESC);

                        grp_id_str = cmd_input[2];

                        grp_id = getChoice(grp_id_str);
                        if(grp_id == -1){
                            send_msg = do_nothing_hrader;
                            send_msg.push_back(ESC);
                            break;
                        }

                        send_msg.append(grp_id_str);
                        send_msg.push_back(ESC);

                    }else{ 
                        send_msg = do_nothing_hrader;
                        send_msg.push_back(ESC);
                    }
                }

            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            }

            break;   

        case LIST_FILE:
            
            if(globle_info->login_status){
                send_msg.append(to_string(globle_info->curr_log_in_id));
                send_msg.push_back(ESC);
            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            } 

            break; 

        case DOWNLOAD_FILE:
            
            if(globle_info->login_status){

                // cout<<"\nEnter File_No: ";
                // cin>>file_no_str;

                // file_no = getChoice(file_no_str);

                // send_msg.append(to_string(file_no));
                // send_msg.push_back(ESC);

                if(cmd_input_size >=4){

                    grp_id_str = cmd_input[1];

                    grp_id = getChoice(grp_id_str);
                    if(grp_id == -1){
                        send_msg = do_nothing_hrader;
                        send_msg.push_back(ESC);
                        break;
                    }

                    send_msg.append(to_string(grp_id));
                    send_msg.push_back(ESC);

                    send_msg.append(cmd_input[2]);
                    send_msg.push_back(ESC);

                    send_msg.append(cmd_input[3]);
                    send_msg.push_back(ESC);

                }else{
                  
                    send_msg = do_nothing_hrader;
                    send_msg.push_back(ESC);
                
                }
            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            } 

            break; 

        case SHOW_DOWNLOAD:

            if(globle_info->login_status){
                send_msg.append(to_string(globle_info->curr_log_in_id));
                send_msg.push_back(ESC);
            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            }
            break;

        case LEAVE_GROUP:
            if(globle_info->login_status){
                if(cmd_input_size >=1){

                    grp_id_str = cmd_input[1];

                    grp_id = getChoice(grp_id_str);
                    if(grp_id == -1){
                        send_msg = do_nothing_hrader;
                        send_msg.push_back(ESC);
                        break;
                    }

                    send_msg.append(to_string(grp_id));
                    send_msg.push_back(ESC);

                    send_msg.append(to_string(globle_info->curr_log_in_id));
                    send_msg.push_back(ESC);
                }else{
                    send_msg = do_nothing_hrader;
                    send_msg.push_back(ESC);
                }

            }else{
                send_msg = do_nothing_hrader;
                send_msg.push_back(ESC);
            }
            break;


        default:
            send_msg = do_nothing_hrader;
            send_msg.push_back(ESC);
            break;


        // EXTRA

        case SHOW_ALL_INFO:
            break;
    
    }

    return true;
}

bool getLderIpPort(int grp_id, vector<string> & recv_msg_vector, string & lder_ip, string & lder_port){
    
    lder_ip.clear();
    lder_port.clear();

    int vlen = recv_msg_vector.size();

    bool flag = false;

    string id_str;
    int id;

    int counter = 0;

    for(int i=1; i<vlen; i++){
        id_str.clear();

        for(int j=0; j<recv_msg_vector[i].length(); j++){
            if(recv_msg_vector[i][j] == ' '){

                id = stoi(id_str);
                if(id == grp_id){
                    flag = true;
                }else{
                    break;
                }
            }else{
                id_str.push_back(recv_msg_vector[i][j]);
            }
        }

        if(flag){
            id_str.clear();

            for(int j=0; j<recv_msg_vector[i].length(); j++){
                
                if(recv_msg_vector[i][j] == ' '){
                    counter++;

                    if(counter == 3){
                        lder_ip = id_str;
                    }else if( counter == 4){
                        lder_port = id_str;
                    }
                    
                    id_str.clear();
                    
                }else{
                    id_str.push_back(recv_msg_vector[i][j]);
                }
            }

            break;
        }
    }

    return flag;
}

// peer client thread fun
class support_info_class{
    public:
        int file_id;
        string file_name;
        int file_size;

        string dwn_path;
        string up_path;

        int seekpoint;
        int data_len;
};

class peer_dwn_file_info_class{
    public:

        int chunk;
        int counter;

        bool possible;
};


struct data_peerClient_struct{
    string serv_ip;
    string serv_port;

    pthread_t serv_join;
    support_info_class * support_info;
    peer_dwn_file_info_class * dwn_info;
};


map<string, peer_dwn_file_info_class *> dwn_files_info;
map<int, vector<string>> peer_dwn_file;

void * peerClient( void * input ){

    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);

    data_peerClient_struct * data_peerClient = (data_peerClient_struct *)input;
    support_info_class * support_info = data_peerClient->support_info; 

    peer_dwn_file_info_class * dwn_info = data_peerClient->dwn_info;

    // pthread_t serv_join = data_peerClient->serv_join;
    // globle_info->serv_join.push_back(serv_join);


    // string filepath = "./terminal/" + to_string(support_info->seekpoint) + ".txt";
    // string str;
    // FILE * t_ptr = fopen(filepath.c_str(), "w+");

    int return_val;
    // cout<<"PeerClient Running"<<endl;    
    // str = "peerClient <" + to_string(support_info->seekpoint)  + ">\n";
    // fputs(str.c_str(), t_ptr);
    // fclose(t_ptr);


    // sleep(2);

    struct addrinfo hints;
    struct addrinfo *servinfo, *rev;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    return_val = getaddrinfo(data_peerClient->serv_ip.c_str(), data_peerClient->serv_port.c_str(), &hints, &servinfo);

    // t_ptr = fopen(filepath.c_str(), "a+");
    // str = "peerClient <" + data_peerClient->serv_ip + "," + data_peerClient->serv_port + ">" + "\n";
    // fputs(str.c_str(), t_ptr);
    // fclose(t_ptr);

    if(return_val != 0){
        perror(gai_strerror(return_val));
        return 0;
    }

    int sockfd; // Socket File Descriptor
    int positive = 1;

    for(rev = servinfo; rev != NULL; rev = rev->ai_next){

        sockfd = socket(rev->ai_family, rev->ai_socktype, rev->ai_protocol);
        if(sockfd == -1){
            perror("SOCKET");
            continue;
        }        

        return_val = connect(sockfd, rev->ai_addr, rev->ai_addrlen);
        if(return_val == -1){
            close(sockfd);
            perror("CONNECT");
            continue;
        }

        break;
        // To bind to first match
    }

    freeaddrinfo(servinfo);

    if(rev == NULL){
        perror("NO MATCH: BIND");
        exit(1);
    }

    // t_ptr = fopen(filepath.c_str(), "a+");
    // str = "ALL done: ";
    // str.append("Msg Making: \n");
    // fputs(str.c_str(), t_ptr);
    // fclose(t_ptr);

    string recv_msg;
    string send_msg;
    string serv_msg;

    string choice_str;

    int choice;
    bool flag=true;

    char tf_flag;

    // WORKING
    send_msg = to_string(support_info->file_id);
    send_msg.push_back(ESC);

    send_msg.append(support_info->up_path);
    send_msg.push_back(ESC);

    send_msg.append(to_string(support_info->seekpoint));
    send_msg.push_back(ESC);

    send_msg.append(to_string(support_info->data_len));
    send_msg.push_back(ESC);

    // t_ptr = fopen(filepath.c_str(), "a+");
    // str = "Msg made: " + send_msg + "\n";
    // fputs(str.c_str(), t_ptr);
    // fclose(t_ptr);

    flag = sendMsg(sockfd, send_msg);

    // string file_path = globle_info->curr_path + "/" + support_info->file_name;
    string file_path = support_info->dwn_path;

    // t_ptr = fopen(filepath.c_str(), "a+");
    // str = "File Path: " + file_path + "\n";   
    // fputs(str.c_str(), t_ptr);
    // fclose(t_ptr);
    
    if(flag){

        flag = recvMsg(sockfd, recv_msg);


        // t_ptr = fopen(filepath.c_str(), "a+");
        // str = "\n\n" + recv_msg + "\n\n";   
        // fputs(str.c_str(), t_ptr);
        // fclose(t_ptr);

        if(flag){

            // ofstream outfile (file_path.c_str(), ofstream::binary);

            // // write to outfile
            // outfile.seekp(support_info->seekpoint, outfile.beg);
            // outfile.write(recv_msg.c_str(),support_info->data_len);

            // outfile.close();
            // infile.close();

            FILE  * ptr = fopen(file_path.c_str(), "r+");

            fseek(ptr, support_info->seekpoint, SEEK_SET);
            fputs(recv_msg.c_str(), ptr);

            fclose(ptr);
        }   
    }
    
    if(!flag){
        cout<<"\n\nProblem\n";
        dwn_info->possible = false;
    }else{
        // cout<<"chunk <"<<support_info->seekpoint<<"> done"<<endl;
        dwn_info->counter++;
    }

    // t_ptr = fopen(filepath.c_str(), "a+");
    // str = "Conversation Complete\n";
    // fputs(str.c_str(), t_ptr);
    // fclose(t_ptr);
        
    close(sockfd);

    // globle_info->serv_join.erase( remove(globle_info->serv_join.begin(), globle_info->serv_join.end(), serv_join), globle_info->serv_join.end() );
    pthread_exit(NULL);
}

bool makeDownloadFile(string file_info_str, string dwn_path){

    // cout<<"1 start"<<endl;

    vector<string> file_info;
    int file_info_len = stringToVector(file_info_str, file_info, ',');

    int file_id = stoi(file_info[0]);
    string file_name = file_info[1];
    int file_size = stoi(file_info[2]);

    vector<string> file_path_temp;
    int file_path_temp_len = stringToVector(file_name, file_path_temp, '/');

    string file_path;

    if(file_info_len > 1){
        file_path = dwn_path + "/" + file_path_temp[file_path_temp.size()-1];
    }else{
        file_path = dwn_path + "/" + file_name;
    }
    // string path_str = globle_info->curr_path

    // cout<<"1 start : "<<file_path<<endl;

    vector<char> empty(1024, '0');
    ofstream ofs(file_path.c_str(), std::ios::binary | std::ios::out);

    for(int i = 0; i < file_size/1024; i++)
    {
        if (!ofs.write(&empty[0], empty.size()))
        {
            return false;
        }
    }

    int remaining = file_size - ((file_size/1024) * 1024);
    empty = vector<char>(remaining, '0');

    if (remaining > 0 && !ofs.write(&empty[0], remaining))
    {
        return false;
    }

    // cout<<"1 end"<<endl;

    return true;
}

bool doDownload(vector<string> & recv_msg_vector, string dwn_path){

    vector<string> file_info;
    int file_info_len = stringToVector(recv_msg_vector[2], file_info, ',');


    vector<string> file_path_temp;
    stringToVector(file_info[1], file_path_temp, '/');

    // string path_str = globle_info->curr_path;
    string file_name_temp = file_path_temp[file_path_temp.size()-1];

    int file_id = stoi(file_info[0]);
    string file_name = file_name_temp;
    int file_size = stoi(file_info[2]);

    int recv_vector_len = recv_msg_vector.size();

    vector<string> server;

    int peer_id;
    string ip;
    string port;

    int return_val;

    int remaining = file_size;
    int seek_point = 0;
    int ind;

    int chunk_len;
    bool flag;

    peer_dwn_file_info_class * dwn_info = new peer_dwn_file_info_class;
    dwn_info->chunk = ceil((float)file_size/(CHUNK_SIZE));
    dwn_info->counter = 0;

    dwn_files_info.insert({file_name, dwn_info});
    
    if(peer_dwn_file.find(globle_info->curr_log_in_id) == peer_dwn_file.end() ){
        vector<string> temp;

        temp.push_back(file_name);
        peer_dwn_file.insert({globle_info->curr_log_in_id, temp});
    }else{
        peer_dwn_file[globle_info->curr_log_in_id].push_back(file_name);
    }


    if(recv_vector_len > 2){
        // cout<<endl<<"\nIPS:\n";

        ind = 3;
        while(remaining > 0){

            flag = stringToVector(recv_msg_vector[ind], server, ' ');
            
            if(remaining >= CHUNK_SIZE){
                chunk_len = CHUNK_SIZE;
            }else{
                chunk_len = remaining;
            }
            
            data_peerClient_struct * data = new data_peerClient_struct;
            support_info_class * support_info = new support_info_class;

            support_info->file_id = file_id;
            support_info->file_name=file_name;
            support_info->file_size = file_size;

            support_info->dwn_path = dwn_path + "/" + file_name;
            support_info->up_path = file_info[1];

            support_info->data_len = chunk_len;
            support_info->seekpoint = seek_point;

            data->dwn_info = dwn_info;            
            data->serv_ip = server[1];
            data->serv_port = server[2]; 
            data->support_info = support_info;

            return_val = pthread_create(&data->serv_join, NULL, &peerClient, (void *)data);            

            seek_point += CHUNK_SIZE;
            remaining -= CHUNK_SIZE;

            ind++;
            if(ind >= recv_vector_len){
                ind = 3;
            }
        }
    }else{
        return false;
    }

    return true;
}

// server msg
bool getServerMsg(string & send_msg, string & recv_msg, string & serv_msg){

    serv_msg.clear();

    vector<string> recv_msg_vector;
    int recv_vector_len = getMsgVector(recv_msg, recv_msg_vector);
    
    vector<string> send_msg_vector;
    int send_vector_len = getMsgVector(send_msg, send_msg_vector);
    
    int recv_choice = stoi(recv_msg_vector[0]);
    int send_choice = stoi(send_msg_vector[0]);
    
    // cout<<"SEND choice: "<<send_choice;

    // variables

    char path[4096];
    string path_str;    

    int return_val;
    bool return_flag;

    string grp_id_str;

    int peer_id;
    int file_id;
    int grp_id;

    char tf_flag;

    usr_info_class * new_usr;
    usr_info_class * usr;

    file_info_class * file_info;

    string str1;
    string str2;

    vector<int> own_groups;

    string grp_ldr_id;
    string grp_ldr_ip;
    string grp_ldr_port;

    bool flag;

    int len;

    peer_dwn_file_info_class * dwn_file_info_var;
    vector<string> file_names;
 
    switch(recv_choice){
        case EXIT:

            if(globle_info->login_status)
                pthread_cancel(globle_info->serv_thread);
            return false;

        case REGISTER:
            new_usr = getNewUsrInfo(stoi(recv_msg_vector[1]));
            local_users.insert({send_msg_vector[1], new_usr});

            serv_msg.append("Success, Peer_id = ");
            serv_msg.append(recv_msg_vector[1]);
            
            // getcwd(path, sizeof(path));
            
            // path_str = path;
            // path_str.push_back('/');
            // path_str.append("peer");
            // path_str.append(recv_msg_vector[1]);
            // // path_str.push_back('/');

            // globle_info->curr_path = path_str;

            // mkdir(path_str.c_str(), 0777);

            break;

        case LOG_IN:

            tf_flag = recv_msg_vector[1][0];

            if(tf_flag == 'T'){
                serv_msg.append("Success, Logged in, Server Started");

                globle_info->login_status = true;
                globle_info->curr_log_in_id = stoi(send_msg_vector[1]);
                globle_info->user_name = send_msg_vector[2];

                // server start
                return_val = pthread_create(&globle_info->serv_thread, NULL, &peerServer, NULL);
    
            }else{
                serv_msg.append("Failed, Wrong Password");
            }    

            break;

        
        case LOG_OUT:

            tf_flag = recv_msg_vector[1][0];
            if(tf_flag == 'T'){
                serv_msg.append("Success, Logged out, Server stopping");

                globle_info->login_status = false;
                
                // server start                
                pthread_cancel(globle_info->serv_thread);
                close(globle_info->sockfd);
            }

            break;

        case CREATE_GROUP:

            if(stoi(recv_msg_vector[1]) != -1){
                usr = local_users[globle_info->user_name];
                usr->addGroup(stoi(recv_msg_vector[1]));
                
                serv_msg.append("Success, Group_id = ");
                serv_msg.append(recv_msg_vector[1]);
            }else{                
                serv_msg.append("Fail, Group_id exist.");
            }
            
            break;

        case LIST_GROUP:

            serv_msg.append("'LIVE' GROUPs:\n");
            // serv_msg.append("id lderID lderIP lderPORT\n");

            if(recv_vector_len > 0){
                for(int j=1; j<recv_vector_len;j++){
                    serv_msg.append(recv_msg_vector[j] + "\n");
                }
            }else{
                serv_msg.append("No 'Live' groups\n");
            }             
            break;

        case JOIN_GROUP:

            tf_flag = recv_msg_vector[1][0];
            if(tf_flag == 'T'){
                serv_msg.append("Request Made");
            }else{
                serv_msg.append("Wrong input");
            }                               
            break;

        case LIST_GROUP_JOIN_REQ:

            serv_msg.append("Pending Requests:\n");

            if(recv_vector_len > 1){
                for(int j=1; j<recv_vector_len; j++){
                    serv_msg.append(recv_msg_vector[j] + "\n");
                }
            }else{
                serv_msg.append("No Requests till now");
            }

            break;

        case REPLY_JOIN_REQ:

            tf_flag = recv_msg_vector[1][0];
            if(tf_flag == 'T'){
                serv_msg.append("Peer added");
            }else{
                serv_msg.append("Wrong input");
            }                               
            break;

        case UPLOAD_FILE:

            file_id = stoi(recv_msg_vector[1]);

            file_info = getNewFile(file_id, send_msg_vector[2], stoi(send_msg_vector[3]) );
            globle_info->addFile(file_id, file_info);

            serv_msg.append("File Uploded");

            break;    

        case LIST_FILE:

            serv_msg.append("Files:\n");

            if(recv_vector_len > 1){
                for(int j=1; j<recv_vector_len-1; j++){
                    serv_msg.append(recv_msg_vector[j] + "\n");
                }
                
                serv_msg.append("\nAll Unique Files:\n");
                serv_msg.append(recv_msg_vector[recv_vector_len-1] + "\n");
            }else{
                serv_msg.append("No files");
            }

            break;

        case DOWNLOAD_FILE:

            // cout<<recv_msg<<endl;

            tf_flag = recv_msg_vector[1][0];
            if(tf_flag == 'T'){

                if(recv_vector_len>=2 && send_vector_len >=3){
                
                    flag = makeDownloadFile(recv_msg_vector[2], send_msg_vector[3]);

                    // str1=recv_msg_vector[2];
                    // str2=SEND_msg_vector[3];

                    // flag = false;
                    if(flag){
                        flag = doDownload(recv_msg_vector, send_msg_vector[3]);

                        if(flag){
                            serv_msg.append("Download started in background");
                        }else{
                            serv_msg.append("Problem");
                        }
                    }else{
                        serv_msg.append("Problem");
                    }
                }
            }else{
                serv_msg.append("Wrong input");
            }                               
            break;

        case SHOW_DOWNLOAD:
            peer_id = globle_info->curr_log_in_id;

            file_names = peer_dwn_file[peer_id];
            len = file_names.size();

            for(int k=0;k<len;k++){
                dwn_file_info_var = dwn_files_info[file_names[k]];
                
                // cout<<dwn_file_info_var->chunk<<" : "<<dwn_file_info_var->counter;
                serv_msg.push_back( dwn_file_info_var->chunk == dwn_file_info_var->counter ? 'C' : 'D' );
                serv_msg.push_back(' ');
                serv_msg.append(file_names[k]);
                serv_msg.push_back('\n');
            }

            break;

        case LEAVE_GROUP:
            tf_flag = recv_msg_vector[1][0];
            if(tf_flag == 'T'){
                serv_msg.append("Left group");
            }else{
                serv_msg.append("Wrong input");
            }                               
            break;


        default:
            serv_msg = "Do nothing / Failed / Wrong input / Not Logged In";
            send_msg.push_back(ESC);
            break;
            

        // EXTRA

        case SHOW_ALL_INFO:
            serv_msg.append(recv_msg_vector[1]);
            break;
    
    }

    return true;
}

int getChoiceFromStr(string str){
    // cout<<str;
    
    if(str.compare("create_user") == 0){
        return REGISTER;

    }else if(str.compare("login") == 0){
        return LOG_IN;

    }else if(str.compare("create_group") == 0){
        return CREATE_GROUP;

    }else if(str.compare("join_group") == 0){
        return JOIN_GROUP;

    }else if(str.compare("leave_group") == 0){
        return LEAVE_GROUP;

    }else if(str.compare("requests") == 0){
        return LIST_GROUP_JOIN_REQ;

    }else if(str.compare("accept_request") == 0){
        return REPLY_JOIN_REQ;

    }else if(str.compare("list_groups") == 0){
        return LIST_GROUP;

    }else if(str.compare("list_files") == 0){
        return LIST_FILE;

    }else if(str.compare("upload_file") == 0){
        return UPLOAD_FILE;

    }else if(str.compare("download_file") == 0){
        return DOWNLOAD_FILE;

    }else if(str.compare("logout") == 0){
        return LOG_OUT;

    }else if(str.compare("show_downloads") == 0){
        return SHOW_DOWNLOAD;

    }else if(str.compare("stop_share") == 0){
        return LOG_OUT;

    }else if(str.compare("all_info") == 0){
        return SHOW_ALL_INFO;

    }else if(str.compare("exit") == 0){
        return EXIT;

    }else{
        return DO_NOTHING;    
    }
}

// ----------------------------------- Main
int main(int argc, char * argv[]){

    string ip_port_str = argv[1];
    string tracker_info_file_path = argv[2];

    vector<string> ip_port_vect;
    stringToVector(ip_port_str, ip_port_vect, ':');

    stringToCharArr(ip_port_vect[0], &self_server_info->ip);
    stringToCharArr(ip_port_vect[1], &self_server_info->port);

    cout<<"SELF:\n";
    cout<<"IP: "<<self_server_info->ip<<endl;
    cout<<"PORT: "<<self_server_info->port<<endl;

    FILE * tracker_info_file_ptr = fopen(tracker_info_file_path.c_str(), "r");

    char ch;
    string tracker_info_str;
    while((ch = fgetc(tracker_info_file_ptr)) != EOF){
        tracker_info_str.push_back(ch);
    }

    fclose(tracker_info_file_ptr);

    stringToVector(tracker_info_str, ip_port_vect, ':');
    string tracker_ip = ip_port_vect[0];
    string tracker_port = ip_port_vect[1];

    cout<<"\nTRACKER:\n";
    cout<<"IP: "<<tracker_ip<<endl;
    cout<<"PORT: "<<tracker_port<<endl;

    int return_val;
    cout<<"\nPeer Running"<<endl;    

    struct addrinfo hints;
    struct addrinfo *servinfo, *rev;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    return_val = getaddrinfo(tracker_ip.c_str(), tracker_port.c_str(), &hints, &servinfo);

    if(return_val != 0){
        perror(gai_strerror(return_val));
        return 0;
    }

    int sockfd; // Socket File Descriptor
    int positive = 1;

    for(rev = servinfo; rev != NULL; rev = rev->ai_next){

        sockfd = socket(rev->ai_family, rev->ai_socktype, rev->ai_protocol);
        if(sockfd == -1){
            perror("SOCKET");
            continue;
        }        

        return_val = connect(sockfd, rev->ai_addr, rev->ai_addrlen);
        if(return_val == -1){
            close(sockfd);
            perror("CONNECT");
            continue;
        }

        break;
        // To bind to first match
    }

    freeaddrinfo(servinfo);

    if(rev == NULL){
        perror("NO MATCH: BIND");
        exit(1);
    }

    string recv_msg;
    string send_msg;
    string serv_msg;

    string choice_str;

    int choice;
    bool flag=true;

    // SETUP

    vector<string> cmd_input;
    string raw_input;
    // char raw_input_chr[2048];

    // WORKING
    while(flag){
       
        // cout<<getPeerMenu()<<": ";
        getline(cin,raw_input);
        
        return_val = stringToVector(raw_input, cmd_input, ' ');

        if(cmd_input.size() > 0){
            choice = getChoiceFromStr(cmd_input[0]);

            if(choice == -1){
                choice = DO_NOTHING;
            }

            flag = getSendMsg(choice, send_msg, cmd_input);

            if(flag){
                flag = sendMsg(sockfd, send_msg);

                if(flag){
                    flag = recvMsg(sockfd, recv_msg);

                    if(flag){
                        flag = getServerMsg(send_msg, recv_msg, serv_msg);
                        cout<<endl<<endl<<"---->  Server Reply: "<<endl;
                        cout<<serv_msg<<endl;
                    }
                }
            }else{
                sendMsg(sockfd, send_msg);
            }

            cout<<endl;
        }
    }

    cout<<"Conversation Complete"<<endl;    
    close(sockfd);


    // pthread_t server_thread;
    if(globle_info->login_status)
        pthread_cancel(globle_info->serv_thread);

    vector<pthread_t> pending_req = globle_info->serv_join;
    for(int i=0; i < pending_req.size(); i++ ){
        pthread_cancel(pending_req[i]);
    }

    cout<<"cancelled"<<endl;

    return 0;
}