/////////////////////////////////////////////////////////////////////////////
//                         Single Threaded Networking
//
// This file is distributed under the MIT License. See the LICENSE file
// for details.
/////////////////////////////////////////////////////////////////////////////

// code base from Nick Sumner github repo: https://github.com/nsumner/web-socket-networking"
// code modified by Anh Vo

#include <iostream>
#include <unistd.h>


#include "Client.h"

int
main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: \n  " << argv[0] << " <ip address> <port>\n"
              << "  e.g. " << argv[0] << " localhost 4002\n";
    return 1;
    }
    
    networking::Client client{argv[1], argv[2]};
    while (!client.isDisconnected()) {
        client.update();

        // Check for received messages from the server
        std::string message = client.receive();
        if (!message.empty()) {
            // Process the received message (game-specific logic)
            // For now, just print the message
            std::cout << "Received from server: " << message << std::endl;
        }

        // Implement client-specific logic here

        // Test: Send a message to the server
         client.send("Hello, server!");
    }

    return 0;
}
   





