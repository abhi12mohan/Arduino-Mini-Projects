# Arduino Mini-Projects

## Overview
### 1. Bop-It Game - A unique take on the traditional "Bop-It" toy. 
The player must complete one of the following actions within the provided time interval: "Bop" (press a button), shout, tilt, or spin. Each correct successive completion of a task corresponds to another point to the player's score. The player continues the game until they incorrectly complete the presented action or run out of time. The player's score is then sent and stored to a database, from which the player can see their current best score, in addition to their current standing in relation to other "Bop-It" players.

Link to demonstration: https://www.youtube.com/watch?v=l-Du1DKhX9U&ab_channel=AbhishekMohan

### 2. Clapper Signal - A simple tool that changes a setting with a double-clap. 
In this example, the system essentially detects for a "double-clap" (2 consecutive claps) within a span of 0.75 seconds. If such a signal is detected, the system's screen changes color. However if there is a continuous sound signal (time span does not matter), the signal will not be confirmed and the screen's color will not be changed. This can then be extended to other embedded systems in which the user wants to change a particular setting by simply clapping.

Link to demonstration: https://www.youtube.com/watch?v=SnWF85daGQ4&ab_channel=AbhishekMohan

### 3. Simple Key Exchange - A system to safely retrieve and display interesting facts.
There are 2 main components to this project: info encryption and info retrieval. For encryption, the system uses a vigenere/caeser cipher to first encrypt the provided "keyword," which in our current example is a number. Upon encrypting the keyword, the server receives this keyword, decrypts it, and uses it to provide the user with some information. In this case, we use a Wikipedia API that essentially generates a random interesting fact pertaining to the keyword. Then, the Arduino system receives the resulting (eventually encrypted) information, decrypts it, and displays it for the user.

Link to demonstration: https://www.youtube.com/watch?v=skxSyK2bIMk&ab_channel=AbhishekMohan
