# Security Features

There are a couple important security features within this app to keep your information secure.

## Cryptography

All traffic between server and client is encrypted through SSL using the servers key and certificate. It currently uses AES encryption.

Passwords are hashed using SHA256 by the client and again before placing it in the database with salt to prevent attacks that listen over the network and rainbow tables. One small downside with this is that the server cant really protect the user from having a poor password. Only client side validation is possible for checking password health.

One thing that is missing is a third party provider for issueing a certificate to the server and having the client validate that certificate through the third party provider. This is a feature that should be added to increase security later

## General Security

- All server requests are blocked until the user signs in to protect the program from unforeseen security issues.

- All malformed requests are ignored by the server

- Each command has a documented list of requirements it protects against in the codebase. Here is an example, if you want to check out the other features check out `server-commands.c` and `client-commands.c`
```c
/**
 * @brief Callback for processing private messages
 *
 * REQUIRES:
 *      - user must to be logged in
 *      - contains 3 arguments. [commandname, usertosend, message]
 *      - recipient must be logged in
 *      - message must not be empty
 *      - cant talk to yourself
 *
 * @param args [commandname, usertosend, message]
 * @param argc 3
 * @return int
 */
```

## Security Goals

Here is a list of each security goal and how it is blocked:

**Mallory cannot get information about private messages for which she is not either the sender or the intended recipient.**
- The server focuses on sending private messages to only the parties that are allowed to view. 
- The server requires to you send which recipient you intend to send it to and with SSL active, reading the network won't work.
- The database stores who sent and who is a message for so when requesting, you'll only see messages between you and another, unless you log in as another account.

**Mallory cannot send messages on behalf of another user.**
- You must be logged in with your own account to send a message
- You cannot pretend to be another user as the server signs a message with your name when you send it.

**Mallory cannot modify messages sent by other users.**
- All queries that change or request something from the database pass through an sql injection checker

**Mallory cannot find out users’ passwords, private keys, or private messages (even if the server is compromised).**
- All passwords are hashed with SHA256 on the client side and then once again on the server side but with salt.

**Mallory cannot use the client or server programs to achieve privilege escalation on the systems they are running on.**
- The server and client both do not run with sudo active

**Mallory cannot leak or corrupt data in the client or server programs.**
- All malformed messages are ignored by the server

**Mallory cannot crash the client or server programs.**
- Malformed messages are ignored by the server

**The programs must never expose any information from the systems they run on.**
- Only username and password and messages are sent through the wire