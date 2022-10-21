# Chatroom-in-C
A Chatroom service in C. The service splits up on the server side where all the functions taking place and the client side which every user uses. Clients can talk      to one another via connecting to the server.

Specifications:
    The server and the client are connected to 127.0.0.1 ip address.

This service can handle up to 100 clients.    

~REGISTRATION / SIGNUP
    This servive will create .txt file ("contacts.txt") and will save every unique user(Name/Password/GroupName) there (REGISTER).
    If a user will try to join the service and he's data already exist there he will (SignUp) as the same user.

~MESSAGE HISTORY
    Every message(public/private) is saved in [groupname].txt file. 

~CLOSE GROUP
    The first user from every group on ("contacts.txt") file is the ADMIN of his group and only he has the permission to close 
    his group.

~COMMUNICATIONS
    Private messages will only work between same group members.
    If an ADMIN (first group register ) close a group the online user cant communicate together but they can communicate with the
    server of the service.
    
Compile : make

Run : ./server 5000
      ./client 5000

Commands:       Parameters:
        /m      [message]               # SENDS MESSAGE TO ALL GROUP MEMBERS.
        /msg    [name] [message]        # SENDS PRIVATE MESSAGE TO SPECIFIC GROUP MEMBER.
        /close                          # THE ADMIN CLOSE COMMUNICATIONS BETWEEN ALL ONLINE GROUP MEMBERS.
        /chathistory                    # APPEARS ALL THE MESSAGES(PUBLIC+PRIVATE).
        /exit                           # EXIT THE SERVICE.
