
![ft8b4 screen image](https://github.com/gkcambr/ft8b4/blob/main/ft8b4.png?raw=true)
      
# ft8b4
    
The objective of ft8b4 is to provide a free ham radio application that runs on GNU/Linux and performs some of the functions of JT-Alert, a Windows application not available on Linux. The goal is to simplify the selection of which CQ to select for response.  
  
ft8b4 is a lightweight application with the following functions:  
  
  * It filters CQ requests from the received WSJT-X list.  
  * Ft8b4 supports ft8 and ft4 modes.
  * CQs with call signs previously logged by Klog are flagged with the mode, band and date.
  * Duplicate CQs can be eliminated from posting on the ft8b4 terminal if the user chooses.  
  * Users can select a CQ for response by double clicking on the listing in ft8b4.
  
ft8b4 acts as a proxy, operating between WSJT-X and Klog. It is opaque; that is all messages between WSJT-X and Klog are passed without modification. When ft8b4 is opened it loads the Klog sqlite3 database and creates a hash file. It then closes that database to avoid interference with Klog. When subsequent QSOs are logged from WSJT-X, ft8b4 adds those QSOs to the hash.  
  
The application is written in C++ and runs in a terminal window using ncurses for the user interface.
## Websites
    
You can view my station and biography at <https://www.qrz.com/db/KC1ATT>.  
I maintain a web-based SDR site for hams at <http://www.radiocheck.us>.  
## Installation
    
Read the INSTALL.txt file for installation instructions.
