#include <bits/stdc++.h>
#include <netdb.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

// #include <string.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <sys/types.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <sys/wait.h>
// #include <signal.h>

#define PORT "45000"
#define BACKLOG 10
#define MAXDATASIZE 10

#define ESC '\x1b'

#define rep(i,a,b) for(int i=a;i<b;i++)
#define erep(i,a,b) for(int i=a;i<=b;i++)

using namespace std;

// ----------------------------------- const var
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


// extra
const int SHOW_ALL_INFO = 9998;

// ----------------------------------- All Peer info
class peer_info_class{
    public:
        int id;
        bool login_status;

        string user_name;
        string password;

        string ser_ip;
        string ser_port;

        vector<int> files;

        vector<int> othr_group;
        vector<int> own_group;

        void addOwnGrp(int grp_id){
            own_group.push_back(grp_id);
        }

        void addOthrGrp(int grp_id){
            othr_group.push_back(grp_id);
        }

        void addFile(int file_id){
            files.push_back(file_id);
        }
};

peer_info_class * getNewPeerInfo(int p_id, string usr_name, string pass, string peer_ser_ip, string peer_ser_port){
    peer_info_class * new_peer_info = new peer_info_class;

    new_peer_info->id = p_id;
    new_peer_info->login_status = false;
    
    new_peer_info->user_name = usr_name;
    new_peer_info->password = pass;

    new_peer_info->ser_ip = peer_ser_ip;
    new_peer_info->ser_port = peer_ser_port;

    return new_peer_info;
}

class grp_info_class{
    public:
        int id;

        int ldr;
        vector<int> members; 
        vector<int> waiting;

        void addGrpMember(int member_id){
            members.push_back(member_id);
        }

        void addWaitMember(int member_id){
            waiting.push_back(member_id);
        }

};

grp_info_class * getNewGrpInfo(int grp_id, int grp_ldr){
    grp_info_class * new_grp_info = new grp_info_class;

    new_grp_info->id = grp_id;
    new_grp_info->ldr = grp_ldr;

    return new_grp_info;
}

class file_info_class{
    public:
        int id;

        string name;
        int size;
        vector<int> peers;

        void addPeer(int peer_id){
            peers.push_back(peer_id);
        }
};

file_info_class * getNewFileInfo(int file_id, string file_name, int size){
    file_info_class * new_file_info = new file_info_class;

    new_file_info->id = file_id;
    new_file_info->name = file_name;
    new_file_info->size = size;

    return new_file_info;
}

class info_class{
    public: 

        int nxt_peer_id;
        int nxt_file_id;
        int nxt_grp_id;

        map<int, peer_info_class *> peers_info;         
        map<int, file_info_class *> files_info;
        map<int, grp_info_class *> grps_info;

        map<pair<string,int>, vector<int>> ip_port_to_users; 
        map<string,int> file_name_id;

        info_class(){
            nxt_peer_id = 1;
            nxt_file_id = 1;
            nxt_grp_id = 1;
        }

        int addPeer(string usr_name, string pass, string peer_ser_ip, string peer_ser_port){
            
            int peer_id = nxt_peer_id++;
            peer_info_class * new_peer_info = getNewPeerInfo(peer_id, usr_name, pass, peer_ser_ip, peer_ser_port);

            peers_info.insert({peer_id, new_peer_info});
            return peer_id;
        }

        int addFile(string file_name, int peer_id, int size){

            int file_id = nxt_file_id++;
            file_info_class * new_file_info = getNewFileInfo(file_id, file_name, size);
            
            new_file_info->addPeer(peer_id);
            files_info.insert({file_id, new_file_info});
        
            peer_info_class * peer_info = peers_info[peer_id];
            peer_info->addFile(file_id);

            file_name_id.insert({file_name, file_id});

            return file_id;
        }

        int addGrp(int grp_ldr, int grp_id){

            // int grp_id = nxt_grp_id++;
            if(grps_info.find(grp_id) == grps_info.end()){
                grp_info_class * new_grp_info = getNewGrpInfo(grp_id, grp_ldr);
        
                grps_info.insert({grp_id, new_grp_info});

                peer_info_class * grp_ldr_info = peers_info[grp_ldr];
                grp_ldr_info->addOwnGrp(grp_id);

                return grp_id;
            }else{
                return -1;
            }
        }

        bool removeFromGroup(int peer_id, int grp_id){

            bool bool_val = true;

            peer_info_class * peer = peers_info[peer_id];
            peer->othr_group.erase(remove(peer->othr_group.begin(), peer->othr_group.end(), grp_id), peer->othr_group.end());

            grp_info_class * grp = grps_info[grp_id];
            grp->members.erase(remove(grp->members.begin(), grp->members.end(), peer_id), grp->members.end());

            return bool_val;
        }

        void addUserToIpPort(string ip, int port, int peer_id){
            
            if(ip_port_to_users.find({ip,port}) == ip_port_to_users.end()){
                vector<int> peer_ids;

                peer_ids.push_back(peer_id);

                ip_port_to_users.insert({{ip,port},peer_ids});
            }else{
                ip_port_to_users[{ip,port}].push_back(peer_id);
            }
        }

        void removeIpPort(string ip, int port){

            int peer_id;
            vector<int> peers = ip_port_to_users[{ip,port}];

            int peer_count = peers.size();
            for(int i=0; i<peer_count; i++){

                peer_id = peers[i];

                vector<int> files = peers_info[peer_id]->files;
                int file_count = files.size();
                int file_id;

                for(int j=0;j<file_count;j++){

                    file_id = files[j];

                    file_info_class * file = files_info[file_id];

                    if( file->peers.size() == 1 ){
                        files_info.erase(file_id);
                    }else{
                        file->peers.erase(remove(file->peers.begin(), file->peers.end(), file_id), file->peers.end());
                    }

                }

                vector<int> othr_grp = peers_info[peer_id]->othr_group;
                int grp_count = othr_grp.size();
                int grp_id;

                for(int j=0;j<grp_count;j++){

                    grp_id = othr_grp[j];
                    grp_info_class * grp = grps_info[grp_id];

                    grp->members.erase(remove(grp->members.begin(), grp->members.end(), peer_id), grp->members.end());
                }

                vector<int> own_grp = peers_info[peer_id]->own_group;
                grp_count = own_grp.size();
                grp_id;

                for(int j=0;j<grp_count;j++){

                    grp_id = own_grp[j];
                    grp_info_class * grp = grps_info[grp_id];

                    vector<int> members = grp->members;

                    int mem_id;
                    int member_count = members.size();

                    for(int k=0;k<member_count;k++){
                        mem_id = members[k];

                        peers_info[mem_id]->othr_group.erase(remove(peers_info[mem_id]->othr_group.begin(), peers_info[mem_id]->othr_group.end(), grp_id), peers_info[mem_id]->othr_group.end());
                    }

                    grps_info.erase(grp_id);
                }

                peers_info.erase(peer_id);
            }            

            ip_port_to_users.erase({ip,port});
        }

        // get strings
        vector<int> getPeerGroupList(int peer_id){
            vector<int> return_vect;
            
            // user
            peer_info_class * peer_info;

            vector<int> othr_group;
            vector<int> own_group;

            // group
            grp_info_class * grp_info;
            peer_info_class * grp_ldr_info;

            int grp_ldr;
            int grp_id;

            peer_info = peers_info[peer_id];

            othr_group = peer_info->othr_group;
            own_group = peer_info->own_group;

            if(othr_group.size() > 0){
                for(int j=0; j<othr_group.size(); j++){
            
                    grp_id = othr_group[j];
                    grp_info = grps_info[grp_id];
                    
                    grp_ldr = grp_info->ldr;
                    grp_ldr_info = peers_info[grp_ldr];

                    if( grp_ldr_info->login_status ){

                        return_vect.push_back(grp_id);
                    }
                }
            }

            if(own_group.size() > 0){                
                for(int j=0; j<own_group.size(); j++){
                    return_vect.push_back(own_group[j]);
                }
            }

            return return_vect;
        }

        string getGroupList(){
            string return_str;
            
            grp_info_class * grp_info;
            peer_info_class * grp_ldr_info;

            int grp_ldr;
            int grp_id;

            for( auto itr = grps_info.begin(); itr != grps_info.end(); itr++){
                grp_id = itr->first;
                grp_info = itr->second;

                grp_ldr = grp_info->ldr;
                grp_ldr_info = peers_info[grp_ldr];

                if( grp_ldr_info->login_status ){

                    return_str.append( to_string(grp_id) +" " );
                    return_str.append( to_string(grp_ldr) +" " );
                    return_str.append( grp_ldr_info->ser_ip +" " );
                    return_str.append( grp_ldr_info->ser_port +" " );
                    return_str.push_back(ESC);
                }
            }

            return return_str;
        }

        bool getFilePeerList(int grp_id, string file_name, vector<int> & peers, file_info_class ** file){
            
            vector<int> nothing;
            int file_id = file_name_id[file_name];

            // cout<<file_id<<endl;
            
            if(files_info.find(file_id) == files_info.end()){
                return false;
            }else{
                peers = files_info[file_id]->peers;
                *file = files_info[file_id];
                return true;
            }
        }

        // Extra
        string getAllInfo(){

            string return_str;

            // user
            int peer_id;
            peer_info_class * peer_info;

            int id;
            bool login_status;

            string user_name;
            string password;

            string ser_ip;
            string ser_port;

            vector<int> files;

            vector<int> othr_group;
            vector<int> own_group;

            // file
            file_info_class * file_info;

            string file_name;
            vector<int> file_peers;

            // group
            grp_info_class * grp_info;

            int grp_ldr;
            vector<int> grp_members;

            // IP PORT
            pair<string, int> ip_port;
            vector<int> user_ids;


            // User
            return_str.append("USERS:\n");
            return_str.append("id login_status uname pass ip port [files] [othergrp] [owngrp]\n");

            for( auto itr = peers_info.begin(); itr != peers_info.end(); itr++){
                peer_id = itr->first;
                peer_info = itr->second;

                id = peer_info->id;
                login_status = peer_info->login_status;

                user_name = peer_info->user_name;
                password = peer_info->password;

                ser_ip = peer_info->ser_ip;
                ser_port = peer_info->ser_port;

                files = peer_info->files;

                othr_group = peer_info->othr_group;
                own_group = peer_info->own_group;

                return_str.append(to_string(id) + " ");
                return_str.append(to_string(login_status) + " ");
                return_str.append(user_name + " ");
                return_str.append(password + " ");
                return_str.append(ser_ip + " ");
                return_str.append(ser_port + " ");
                
                return_str.push_back('[');
                if(files.size() > 0){
                    
                    for(int j=0; j<files.size(); j++){
                        return_str.append(to_string(files[j]) + ",");
                    }
                    return_str.pop_back();
                }
                return_str.push_back(']');
                return_str.push_back(' ');

                return_str.push_back('[');
                if(othr_group.size() > 0){
                    for(int j=0; j<othr_group.size(); j++){
                        return_str.append(to_string(othr_group[j]) + ",");
                    }
                    return_str.pop_back();
                }
                return_str.push_back(']');
                return_str.push_back(' ');

                return_str.push_back('[');
                if(own_group.size() > 0){
                    for(int j=0; j<own_group.size(); j++){
                        return_str.append(to_string(own_group[j]) + ",");
                    }
                    return_str.pop_back();
                }
                return_str.push_back(']');

                return_str.push_back('\n');
            }

            // File
            return_str.append("\nFILES:\n");
            return_str.append("id name [peers]\n");

            for( auto itr = files_info.begin(); itr != files_info.end(); itr++){
                id = itr->first;
                file_info = itr->second;

                file_name = file_info->name;
                file_peers = file_info->peers;

                return_str.append(to_string(id) + " ");
                return_str.append(file_name + " ");
                
                return_str.push_back('[');
                if(file_peers.size() > 0){
                    
                    for(int j=0; j<file_peers.size(); j++){
                        return_str.append(to_string(file_peers[j]) + ",");
                    }
                    return_str.pop_back();
                }
                return_str.push_back(']');
                return_str.push_back(' ');

                return_str.push_back('\n');

            }

            // Group
            return_str.append("\nGROUP:\n");
            return_str.append("id ldr [members] [waiting]\n");

            for( auto itr = grps_info.begin(); itr != grps_info.end(); itr++){
                id = itr->first;
                grp_info = itr->second;

                grp_ldr = grp_info->ldr;
                grp_members = grp_info->members;

                return_str.append(to_string(id) + " ");
                return_str.append(to_string(grp_ldr) + " ");
                
                return_str.push_back('[');
                if(grp_members.size() > 0){
                    
                    for(int j=0; j<grp_members.size(); j++){
                        return_str.append(to_string(grp_members[j]) + ",");
                    }
                    return_str.pop_back();
                }
                return_str.push_back(']');
                return_str.push_back(' ');

                grp_members = grp_info->waiting;

                return_str.push_back('[');
                if(grp_members.size() > 0){
                    
                    for(int j=0; j<grp_members.size(); j++){
                        return_str.append(to_string(grp_members[j]) + ",");
                    }
                    return_str.pop_back();
                }
                return_str.push_back(']');
                return_str.push_back(' ');

                return_str.push_back('\n');
            }

            // IP-PORT
            return_str.append("\nIP-PORT USERS:\n");
            return_str.append("<ip,port> [users]\n");

            for( auto itr = ip_port_to_users.begin(); itr != ip_port_to_users.end(); itr++){
                ip_port = itr->first;
                user_ids = itr->second;

                return_str.append("<" + ip_port.first + "," + to_string(ip_port.second) + ">" + " ");
                
                return_str.push_back('[');
                if(user_ids.size() > 0){
                    
                    for(int j=0; j<user_ids.size(); j++){
                        return_str.append(to_string(user_ids[j]) + ",");
                    }
                    return_str.pop_back();
                }
                return_str.push_back(']');
                return_str.push_back(' ');

                return_str.push_back('\n');
            }

            return return_str;
        }
};

info_class * database = new info_class;



// ----------------------------------- Structure & it's method incoming connection information 

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
    
    peer_menu.append("1. EXIT\n"); 
    peer_menu.append("2. ONE\n");
    peer_menu.append("3. TWO\n");

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
        if(buffer[i] == '\x1b'){
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
    string header = to_string(msg_len) + '\x1b';

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
// msg to vector
int getRecvMsgVector(string & recv_msg, vector<string> & recv_msg_vector){
    int msg_len = recv_msg.length();
    string msg_chunck;
    
    int vector_len = 0;

    for(int i=0; i<msg_len; i++){
        if(recv_msg[i] == ESC){
            recv_msg_vector.push_back(msg_chunck);
            msg_chunck.clear();
            vector_len++;
        }else{
            msg_chunck.push_back(recv_msg[i]);
        }
    }

    return vector_len;
}

// message rendering
bool getReplyMsg(string & recv_msg, string & reply_msg, info_struct * info){
    reply_msg.clear();

    string recver_ip;
    int recver_port;

    getIpPort(info, recver_ip, recver_port);

    vector<string> recv_msg_vector;
    int vector_len = getRecvMsgVector(recv_msg, recv_msg_vector);

    int choice = stoi(recv_msg_vector[0]);

    // variables
    bool setup_done = false;

    peer_info_class * peer;
    grp_info_class * grp;

    file_info_class * file;

    int peer_id;
    int file_id;
    int grp_id;

    vector<int> vect001;
    int vect001_len;

    vector<int> vect002;
    int vect002_len;

    vector<int> vect003;
    int vect003_len;

    vector<int> vect004;
    int vect004_len;

    vector<int> vect005;
    int vect005_len;

    vector<int> vect006;
    int vect006_len;

    reply_msg = recv_msg_vector[0];
    reply_msg.push_back(ESC);

    bool flag;

    switch(choice){
        case EXIT:
            return false;

        case REGISTER:

            peer_id = database->addPeer(recv_msg_vector[1], recv_msg_vector[2], recv_msg_vector[3], recv_msg_vector[4]);
            database->addUserToIpPort(recver_ip, recver_port, peer_id);

            reply_msg.append(to_string(peer_id));
            reply_msg.push_back(ESC);

            setup_done = true;
            break;

        case LOG_IN:
            
            peer_id = stoi(recv_msg_vector[1]);
            peer = database->peers_info[peer_id];

            if(peer->password.compare(recv_msg_vector[2]) == 0){

                peer->login_status = true;
                reply_msg.push_back('T');
                reply_msg.push_back(ESC);

            }else{
                reply_msg.push_back('F');
                reply_msg.push_back(ESC);
            }

            break;

        case LOG_OUT:

            peer_id = stoi(recv_msg_vector[1]);
            peer = database->peers_info[peer_id];

            peer->login_status = false;
            reply_msg.push_back('T');
            reply_msg.push_back(ESC);

            break;

        case CREATE_GROUP:

            peer_id = stoi(recv_msg_vector[1]);
            grp_id = stoi(recv_msg_vector[2]);
            grp_id = database->addGrp(peer_id, grp_id);

            reply_msg.append(to_string(grp_id));
            reply_msg.push_back(ESC);
        
            break;

        case LIST_GROUP:

            reply_msg.append(database->getGroupList());
            reply_msg.push_back(ESC);            
            
            break;

        case JOIN_GROUP:

            grp_id = stoi(recv_msg_vector[2]);
            peer_id = stoi(recv_msg_vector[1]);

            if(database->grps_info.find(grp_id) == database->grps_info.end() ){
                reply_msg.push_back('F');
                reply_msg.push_back(ESC);
            }else{
                database->grps_info[grp_id]->addWaitMember(peer_id);

                reply_msg.push_back('T');
                reply_msg.push_back(ESC);
            }

            break;

        case LIST_GROUP_JOIN_REQ:

            peer_id = stoi(recv_msg_vector[1]);

            peer = database->peers_info[peer_id];

            vect001 = peer->own_group;
            vect001_len = vect001.size();     
        
            for(int j=0;j<vect001_len;j++){

                grp = database->grps_info[vect001[j]];

                vect002 = grp->waiting;
                vect002_len = vect002.size();

                reply_msg.append(to_string(vect001[j]) + " ");


                reply_msg.push_back('[');
                for(int k=0; k<vect002_len; k++){
                    reply_msg.append(to_string(vect002[k]) + ",");
                }
                reply_msg.pop_back();
                reply_msg.push_back(']');
                
                reply_msg.push_back(ESC);
            }

            break;

        case REPLY_JOIN_REQ:

            grp_id = stoi(recv_msg_vector[1]);
            peer_id = stoi(recv_msg_vector[2]);

            if(database->grps_info.find(grp_id) == database->grps_info.end() ){
                reply_msg.push_back('F');
                reply_msg.push_back(ESC);
            }else{
                vect001 = database->grps_info[grp_id]->waiting;
                vect001_len = vect001.size();

                flag = false;
                for(int j=0; j<vect001_len; j++){
                    if(vect001[j] == peer_id){
                        flag = true;
                        database->grps_info[grp_id]->waiting.erase(database->grps_info[grp_id]->waiting.begin() + j);

                        break;
                    }
                }

                if(flag){
                    if(database->peers_info.find(peer_id) == database->peers_info.end()){
                        reply_msg.push_back('F');
                        reply_msg.push_back(ESC);
                    }else{
                        database->grps_info[grp_id]->members.push_back(peer_id);

                        peer = database->peers_info[peer_id];
                        peer->othr_group.push_back(grp_id);

                        reply_msg.push_back('T');
                        reply_msg.push_back(ESC);
                    }
                }else{
                    reply_msg.push_back('F');
                    reply_msg.push_back(ESC);
                }
            }

            break;
        
        case UPLOAD_FILE:

            peer_id = stoi(recv_msg_vector[1]);
            file_id = database->addFile(recv_msg_vector[2],peer_id,stoi(recv_msg_vector[3]));

            reply_msg.append(to_string(file_id));
            reply_msg.push_back(ESC);

            break;

        case LIST_FILE:
            peer_id = stoi(recv_msg_vector[1]);

            vect001 = database->getPeerGroupList(peer_id);
            vect001_len = vect001.size();

            for(int j=0; j<vect001_len; j++){

                vect004.clear();
                vect005.clear();

                grp_id = vect001[j];
                reply_msg.append(to_string(grp_id) + " {");

                grp = database->grps_info[grp_id];

                vect002 = grp->members;
                vect002_len = vect002.size();

                for(int k=0; k<vect002_len; k++){
                    peer = database->peers_info[vect002[k]];

                    if(peer->login_status){
                        vect003 = peer->files;
                        vect003_len = vect003.size();

                        for(int l=0; l<vect003_len; l++){
                            vect004.push_back(vect003[l]);
                        }
                    }
                }    

                peer = database->peers_info[ grp->ldr ];
                if(peer->login_status){
                    vect003 = peer->files;
                    vect003_len = vect003.size();

                    for(int l=0; l<vect003_len; l++){
                        vect004.push_back(vect003[l]);
                    }
                }

                sort(vect004.begin(), vect004.end());
                vect004_len = vect004.size();

                if(vect004_len > 0){
                    
                    vect005.push_back(vect004[0]);
                    vect006.push_back(vect004[0]);

                    for(int k=1; k<vect004_len; k++){
                        if(vect004[k-1] != vect004[k]){
                            vect005.push_back(vect004[k]);

                            vect006.push_back(vect004[k]);
                        }
                    }

                    vect005_len = vect005.size();
                    for(int k=0; k<vect005_len; k++){
                        file_id = vect005[k];
                        file = database->files_info[file_id];

                        // To be comment
                        reply_msg.append("\n\t");

                        reply_msg.push_back('[');
                        reply_msg.append(to_string(file->id) + ",");
                        reply_msg.append(file->name + ",");
                        reply_msg.append(to_string(file->size));
                        reply_msg.push_back(']');
                        reply_msg.push_back(',');
                    }
                    reply_msg.pop_back();

                }else{
                    reply_msg.append("No files");
                }            

                // To be comment
                reply_msg.push_back('\n');

                reply_msg.push_back('}');
                reply_msg.push_back(ESC);
            }

            // For All unique files
            vect005.clear();

            vect006_len = vect006.size();
            sort(vect006.begin(), vect006.end());

            if(vect006_len > 0){
                vect005.push_back(vect006[0]);
                for(int k=1; k<vect006_len; k++){
                    if(vect006[k-1] != vect006[k]){
                        vect005.push_back(vect006[k]);
                    }
                }
            }

            reply_msg.push_back('{');
            vect005_len = vect005.size();
            for(int k=0; k<vect005_len; k++){

                file_id = vect005[k];
                file = database->files_info[file_id];

                // To be comment
                reply_msg.append("\n\t");
                
                reply_msg.push_back('[');
                reply_msg.append(to_string(file->id) +",");
                reply_msg.append(file->name + ",");
                reply_msg.append(to_string(file->size));

                reply_msg.push_back(']');
                reply_msg.push_back(',');
            }
            reply_msg.pop_back();

            // To be comment
            reply_msg.push_back('\n');
            reply_msg.push_back('}');
            reply_msg.push_back(ESC);

            break;

        case DOWNLOAD_FILE:
            flag = database->getFilePeerList(stoi(recv_msg_vector[1]), recv_msg_vector[2], vect001, &file);

            // flag=false;
            if(flag){
                
                // file info
                reply_msg.push_back('T');
                reply_msg.push_back(ESC);
                
                file_id = file->id;
                
                reply_msg.append(to_string(file_id)+",");
                reply_msg.append(file->name+",");
                reply_msg.append(to_string(file->size));

                reply_msg.push_back(ESC);

                // peer info

                vect001_len = vect001.size();

                for(int j=0; j<vect001_len; j++){

                    peer_id = vect001[j];
                    peer = database->peers_info[peer_id];

                    if(peer->login_status){
                        reply_msg.append(to_string(peer->id));
                        reply_msg.push_back(' ');
                        reply_msg.append(peer->ser_ip);
                        reply_msg.push_back(' ');
                        reply_msg.append(peer->ser_port); 
                        reply_msg.push_back(ESC);
                    }                    
                }
            }else{
                reply_msg.push_back('F');
                reply_msg.push_back(ESC);
            }

            break; 

        case SHOW_DOWNLOAD:
            break;

        case LEAVE_GROUP:
            peer_id = stoi(recv_msg_vector[2]);
            grp_id = stoi(recv_msg_vector[1]);

            flag = database->removeFromGroup(peer_id, grp_id);

            if(flag){
                reply_msg.push_back('T');
                reply_msg.push_back(ESC);
            }else{
                reply_msg.push_back('F');
                reply_msg.push_back(ESC);
            }

            break;

        case SHOW_ALL_INFO:

            reply_msg.append(database->getAllInfo());
            reply_msg.push_back(ESC);

            break;

        default:
            break;
    }

    return true;
}


// --------- Tracker - Peer conversion
void * trackerPeerConversationFun(void *input){
    
    info_struct * info = (info_struct *)input;

    string recver_ip;
    int recver_port;

    getIpPort(info, recver_ip, recver_port);

    cout<<"<"<<recver_ip<<" , "<<recver_port<<"> :: "<<"Conversation start"<<endl;

    string recv_msg;
    string reply_msg;

    bool flag = true;
    while(flag){

        flag = recvMsg(info->newfd, recv_msg);
        cout<<"<"<<recver_ip<<" , "<<recver_port<<"> :: "<<recv_msg<<endl;

        if(flag){
            flag = getReplyMsg(recv_msg, reply_msg, info);

            if(flag){
                flag = sendMsg(info->newfd, reply_msg);
            }
        }       
    }

    database->removeIpPort(recver_ip,recver_port);
    cout<<"<"<<recver_ip<<" , "<<recver_port<<"> :: "<<"Conversion Complete"<<endl;

    close(info->newfd);    
    pthread_exit(NULL);
}

struct tracker_info_struct{
    string ip;
    string port;
};

void * tracker(void * arg){

    tracker_info_struct * tracker_info = (tracker_info_struct *)arg;


    struct addrinfo hints;
    struct addrinfo *servinfo, *rev;

    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // hints.ai_flags = AI_PASSIVE;

    int return_val;
    return_val = getaddrinfo(tracker_info->ip.c_str(), tracker_info->port.c_str(), &hints, &servinfo);

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
    cout<<"SERVER STARETED: \n"<<endl;

    while(true){

        thread_counter++;
        // cout<<"thread_counter: "<<thread_counter<<endl;

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

        return_val = pthread_create(&new_thread, NULL, &trackerPeerConversationFun, (void *)info);
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


// ----------------------------------- Main
int main(int argc, char * argv[]){

    string tracker_info_file_path = argv[1];

    vector<string> ip_port_vect;
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

    tracker_info_struct * tracker_info = new tracker_info_struct;
    tracker_info->ip = tracker_ip;
    tracker_info->port = tracker_port;

    pthread_t n_thread;
    int r_val = pthread_create(&n_thread, NULL, &tracker, (void *)tracker_info);

    
    string input;
    cin>>input;

    while(true){

        if(input.compare("quit") == 0){
            break;
        }else{
            cin>>input;
        }
    }

    pthread_cancel(n_thread);
  
    return 0;
}
