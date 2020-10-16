# Client-Server-TicTacToe

This is Assignment 4 for CSC209. I created a Client-Server based tic tac toe game.
To run the program, please download the code and enter the following in the terminal,

gcc -Wall ticsvr.c

Then please enter,

./a.out -p <port number>
  
where you would enter the port number for the client to listen in to. Also remove "<>".
Then, on a different device or terminal enter,

nc <ip address> <port number>
  
Again, remove "<>".

The first two people who connect to the tic-tac-toe server get to play; any subsequent people connecting only get to watch, unless one of the players disconnects.
Then the person next in line gets to play.
