# Peer-to-Peer Group Based File Sharing System

### Pre-requisites:

Socket Programming, Multi-threading 

## Goal 

Built a group based file sharing system where users can share, download files from the group they belong to. Download should be parallel with multiple pieces from multiple peers. 

## Note

- You have to divide the file into logical “pieces” , wherein the size of each piece should be 512KB 
- Authentication for login needs to be done 
- Error handling to be done 

## Architecture Overview

The Following entities will be present in the network : 

1. Synchronized trackers(2 tracker system) : 
    1. Maintain information of clients with their files(shared by client) to assist the clients for the communication between peers 
    3. Trackers should be synchronized i.e all the trackers if online should be in sync with each other 
4. Clients: 
    1. User should create an account and register with tracker 
    1. Login using the user credentials 
    1. Create Group and hence will become owner of that group 
    1. Fetch list of all Groups in server 
    1. Request to Join Group 
    1. Leave Group 
    1. Accept Group join requests (if owner) 
    1. Share file across group: Share Accorfing to the filename 
    1. Fetch list of all sharable files in a Group 
    1. Download file 
        1. Retrieve peer information from tracker for the file 
        1. Core Part: Download file from multiple peers (different pieces of file from different peers - piece selection algorithm) simultaneously and all the files which client downloads will be shareable to other users in the same group. 
    11. Show downloads 
    11. Stop sharing file 
    13. Stop sharing all files(Logout) 
    14. Whenever client logins, all previously shared files before logout should automatically be on sharing mode 

## Working

1. At Least one tracker will always be online. 
1. Client needs to create an account (userid and password) in order to be part of the network. 
1. Client can create any number of groups(groupid should be different) and hence will be owner of those groups 
1. Client needs to be part of the group from which it wants to download the file 
1. Client will send join request to join a group 
1. Owner Client Will Accept/Reject the request 
1. After joining group ,client can see list of all the shareable files in the group 
1. Client can share file in any group (note: file will not get uploaded to tracker but only the <ip>:<port> of the client for that file) 
1. Client can send the download command to tracker with the group name and filename and tracker will send the details of the group members which are currently sharing that particular file 
10. After fetching the peer info from the tracker, client will communicate with peers about the portions of the file they contain and hence accordingly decide which part of data to take from which peer (You need to design your own Piece Selection Algorithm) 
11. As soon as a piece of file gets downloaded it should be available for sharing 12.After logout, the client should temporarily stop sharing the currently shared files till the next login 
13. All trackers need to be in sync with each other 

## Commands

1. Tracker: 
    1. Run Tracker: ./tracker tracker\_info.txt tracker\_no tracker\_info.txt - Contains ip, port details of all the trackers 
    1. Close Tracker: quit 
2. Client: 
    1. Run Client: ./client  <IP>: <PORT> tracker\_info.txt
    tracker\_info.txt - Contains ip, port details of all the trackers 
    2. Create User Account:  create\_user <user\_id>  <passwd>
    2. Login: login  <user\_id> <passwd>
    2. Create Group:  creat e\_group  <group\_id>
    2. Join Group:  join\_group  <group\_id>
    2. Leave Group:  leave\_group  <group\_id>
    2. List pending join: requests  list\_requests  <group\_id>
    2. Accept Group Joining Request: accept\_request <group\_id> <user\_id>
    2. List All Group In Network:  list\_groups
    2. List All sharable Files In Group:  list\_files <group\_id>
    2. Upload File: upload\_file <file\_path> <group\_id> 
    2. Download File:  download\_file <group\_id> <file\_name>  <destination\_path>
    13. Logout:  logout
    14. Show\_downloads: show\_downloads
        1. Output format:  
            [D] [grp\_id] filename  
            [C] [grp\_id] filename  
            D(Downloading), C(Complete) 
    15. Stop sharing: stop\_share <group\_id> <file\_name>
