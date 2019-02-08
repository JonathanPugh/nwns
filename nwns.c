#include <ncurses.h>
#include <signal.h>

#include "nwns.h"

//Height of top menu bar
#define TOPMENUHEIGHT 1

//DEBUG_MOD macro can be defined to print the network list multiple times for debugging
//#define DEBUG_MOD 2

//Declare Ncurses pad that will display network list
WINDOW *pad;

//Y position of cursor highlight
int y = 0;

//Lock used to atomically refresh the screen
int lock = 0;

//Row position of visible pad
int padpos = 0;

//Max row and column that can be displayed
static int mrow, mcol;

//Head of network linked list
struct netNode * netList;

//Wireless interface
char wInt[20];

int main(int argc, char *argv[]){

  //If no interface is specified, default to wlp3s0
  if (argc == 1){
    printf("No interface specified, using wlp3s0\n");
    strcpy(wInt,"wlp3s0");
  }
  //Use interface if specified
  else if (argc == 2){
    printf("Using interface %s\n", argv[1]);
    strcpy(wInt, argv[1]);
  }
  //End program if too many arguments were entered
  else { 
    printf("Invalid number of arguments\n");
    return 1;
  }

  //Scan for SSIDs
  initList(wInt);

  //initialize curses
  initscr();

  //End program if terminal does not support colors
  if(!has_colors()){
    endwin();
    printf("Terminal does not support colors\n");
    exit(1);
  }

  //Save terminal state
  savetty();

  //Call endwin() on program exit
  atexit((void*)endwin);

  //Clear the terminal window
  clear();

  //Disable line buffering
  cbreak();

  //Enable keyboard input
  keypad(stdscr, TRUE);

  //Initialize Ncurses colors
  start_color();
  use_default_colors();

  //make mvwchgat() non-blocking
  timeout(0);

  //Selected SSID color
  //Light blue (bg)
  init_color (80, 23, 594, 601);
  //Black (fg)
  init_color(0, 0, 0, 0);
  init_pair (90, COLOR_BLACK, 80);

  //Menu color
  //Gray(fg)
  init_color(72, 304, 601, 23);
  //Green(bg)
  init_color(71, 180, 250, 180);
  init_pair(72, 71, 72);

  //Get terminal size
  getmaxyx(stdscr, mrow, mcol);

  //Disable default cursor
  curs_set(0);

  // create pad
  #ifdef DEBUG_MOD
    int rowcount = getNumElements() * DEBUG_MOD;
  #else
    int rowcount = getNumElements();
  #endif

  //Initialize pad and enable keyboard input
  pad = newpad(rowcount, mcol);
  keypad(pad, TRUE);

  //Get head of network linked list
  netList = getNodeHead();  

  //End program if no networks are detected
  if(!netList){
    printf("No networks detected\n");
    return -1;
  }

  //Print network list to pad
  printList();

  //Highlight first row
  mvwchgat(pad, 0, 0, -1, A_NORMAL, 90, NULL);

  //Set pad position
  prefresh(pad, 0, 0, TOPMENUHEIGHT, 0, mrow - 1, mcol - 1);

  //Catch window resize
  signal(SIGWINCH, resizeHandler);

  //Stores keyboard input
  int ch;

  // Loop until the user enters Q/q
  while((ch = wgetch(pad)) != 'q' && ch != 'Q')
  {
    switch (ch)
    {
      case KEY_UP:
        //Unhighlight previous line
        mvwchgat(pad, y, 0, -1, A_NORMAL, 0, NULL);

        //Move cursor up if possible
        if (y > 0)
          y--;

        //Move pad down if needed
        if (padpos > y)
          padpos--;

        //Highlight new line
        mvwchgat(pad, y, 0, -1, A_NORMAL, 90, NULL);

        //Refresh pad
        prefresh(pad, padpos, 0, TOPMENUHEIGHT, 0, mrow - 1, mcol - 1);
        break;

      case KEY_DOWN:
        //Unhighlight previous line
        mvwchgat(pad, y, 0, -1, A_NORMAL, 0, NULL);

        //Move cursor down if possible
        if (y < rowcount - 1)
          y++;

        //Move pad up if needed
        if ( (y >= mrow - TOPMENUHEIGHT) && (padpos <= (y - mrow + TOPMENUHEIGHT)) )
          padpos++;

        //Highlight new line
        mvwchgat(pad, y, 0, -1, A_NORMAL, 90, NULL);

        //Refresh pad
        prefresh(pad, padpos, 0, TOPMENUHEIGHT, 0, mrow-1, mcol - 1);
        break;
    }
  }


  // remove pad and window, clear terminal
  delwin(pad);
  endwin();
  clear();
  refresh();

  //Restore saved terminal state
  //resetty();
  return 0;
}

void resizeHandler(int sig){
  //Atomic CAS used to redraw the window
  if(__sync_bool_compare_and_swap(&lock, 0, 1)){

    //Delete pad and window
    clear();
    wclear(pad);
    endwin();

    //Recreate window
    initscr();
    atexit((void*)endwin);
    cbreak();
    keypad(stdscr, TRUE);
    //make mvwchgat() non-blocking
    timeout(0);
    
    refresh();

    //Get new window dimensions
    getmaxyx(stdscr, mrow, mcol);

    //Recreate pad
    #ifdef DEBUG_MOD
      pad = newpad(getNumElements() * DEBUG_MOD, mcol);
    #else
      pad = newpad(getNumElements(), mcol);
    #endif

    //Print network list to pad, enable keyboard
    printList();
    keypad(pad, TRUE);

    //Move pad if y is too large or pad position is too small
    if (y > (mrow - TOPMENUHEIGHT) || padpos <= (y - mrow) ){
      padpos = y - mrow + 2;
    }

    //Redraw highlighted line
    mvwchgat(pad, y, 0, -1, A_NORMAL, 90, NULL);

    //Refresh pad
    prefresh(pad, padpos, 0, TOPMENUHEIGHT, 0, mrow - 1, mcol - 1);

    //Clear input buffer
    wgetch(pad);

    //Free lock
    lock = 0;
  }
}

void printList(){

  //Print top menu bar
  printw("%-32s %-11s %-17s %9s", "Network name (SSID)", "Frequency", "MAC Address", "Channel" );
  mvwchgat(stdscr, 0, 0, -1, A_NORMAL, 72, NULL);
  refresh();

  //Print the network list DEBUG_MOD times if it is defined
  #ifdef DEBUG_MOD
    int i;

    for(i = 0; i < DEBUG_MOD; i++){
      //Get head of linked list
      netList = getNodeHead();

      //Add network info to pad
      while(netList){
        wprintw(pad, "%-32s %-4dMHz     %-19s %i\n", netList->ssid, netList->freq, netList->mac, getChannel(netList->freq));
        netList = netList->next;
      }
    }
  //Print list once
  #else
    //Get head of linked list
    netList = getNodeHead();

    //Add network info to pad
    while(netList){
      wprintw(pad, "%-32s %-4dMHz     %-19s %i\n", netList->ssid, netList->freq, netList->mac, getChannel(netList->freq));
      netList = netList->next;
    }
  #endif

  refresh();
}
