# Helping Hand User Interface
# Developed for ESE 519, UPenn, Spring 2015
# Team Helping Hand: Peter Gebhard, Chaitali Gondhalekar, Yifeng Yuan

# Imports
import random, serial, time, sys, ctypes, binascii
import pygame
from pygame.locals import *
from menu.menu import *
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import matplotlib.patches as mpatches
import array
from collections import deque
import csv

# Global settings
c_uint8 = ctypes.c_uint8
DEBUG = True
FPS = 15
WINDOWWIDTH = 640
WINDOWHEIGHT = 480
CELLSIZE = 10
assert WINDOWWIDTH % CELLSIZE == 0, "Window width must be a multiple of cell size."
assert WINDOWHEIGHT % CELLSIZE == 0, "Window height must be a multiple of cell size."
CELLWIDTH = int(WINDOWWIDTH / CELLSIZE)
CELLHEIGHT = int(WINDOWHEIGHT / CELLSIZE)
NUM_OBSTACLES = 40

#             R    G    B
WHITE     = (255, 255, 255)
BLACK     = (  0,   0,   0)
RED       = (255,   0,   0)
GREEN     = (  0, 255,   0)
DARKGREEN = (  0, 155,   0)
DARKGRAY  = ( 40,  40,  40)
BGCOLOR = BLACK

UP = 'up'
DOWN = 'down'
LEFT = 'left'
RIGHT = 'right'

# Bit packing structure for inter-device commands
class Command_bits( ctypes.BigEndianStructure ):
    _fields_ = [
                ("hand",    c_uint8, 1 ),
                ("action",  c_uint8, 2 ),
                ("speed",   c_uint8, 3 ),
                ("left",    c_uint8, 1 ),
                ("right",   c_uint8, 1 )
               ]
class Command( ctypes.Union ):
    _fields_ = [
                ("b",      Command_bits ),
                ("asByte", c_uint8    )
               ]
    _anonymous_ = ("b")

# Bit packing structure for inter-device commands
class MenuCommand_bits( ctypes.BigEndianStructure ):
    _fields_ = [
                ("nothing", c_uint8, 4 ),
                ("hand",    c_uint8, 1 ),
                ("up",      c_uint8, 1 ),
                ("down",    c_uint8, 1 ),
                ("select",  c_uint8, 1 )
               ]
class MenuCommand( ctypes.Union ):
    _fields_ = [
                ("b",      MenuCommand_bits ),
                ("asByte", c_uint8    )
               ]
    _anonymous_ = ("b")

# Bit packing structure for inter-device commands
class Response_bits( ctypes.BigEndianStructure ):
    _fields_ = [
                ("nothing",   c_uint8, 5 ),
                ("exit",      c_uint8, 1 ),
                ("plot_mode", c_uint8, 1 ), # 0 for non-plot-mode, ie. game mode; 1 for plot-mode
                ("collision", c_uint8, 1 )
               ]
class Response( ctypes.Union ):
    _fields_ = [
                ("b",      Response_bits ),
                ("asByte", c_uint8    )
               ]
    _anonymous_ = ("b")

# The main Player class (inherits from PyGame Rect)
class Player(pygame.Rect):
    def __init__(self, ident=0, x=0, y=0, size=20, speed=1, ser=None):
        super(Player, self).__init__(x, y, size, size)
        self.ident = ident
        self.direction = RIGHT
        self.lastDirection = RIGHT
        self.speed = speed
        self.ser = ser
        self.lastMoveCollided = False

    # Move the player in the specified direction
    def move(self, direction, obstacles):
        if direction == UP:
            self.move_ip(0,-1)
            if self.playerCollides(obstacles):
                self.lastMoveCollided = True
                self.move_ip(0,1)
            else:
                self.lastMoveCollided = False
        if direction == DOWN:
            self.move_ip(0,1)
            if self.playerCollides(obstacles):
                self.lastMoveCollided = True
                self.move_ip(0,-1)
            else:
                self.lastMoveCollided = False
        if direction == LEFT:
            self.move_ip(-1,0)
            if self.playerCollides(obstacles):
                self.lastMoveCollided = True
                self.move_ip(1,0)
            else:
                self.lastMoveCollided = False
        if direction == RIGHT:
            self.move_ip(1,0)
            if self.playerCollides(obstacles):
                self.lastMoveCollided = True
                self.move_ip(-1,0)
            else:
                self.lastMoveCollided = False

    # Detect player collisions
    def playerCollides(self, obstacles):
        for obs in obstacles:
            if self.colliderect(obs):
                r = Response()
                r.plot_mode = 0
                r.collision = 1
                print r.asByte
                if self.ser is not None:
                    r = Response()
                    r.plot_mode = 0
                    r.collision = 1
                    print r.asByte
                    self.ser.write(array.array('B', [r.asByte]).tostring())
                return True
        return False

# An AI Player used during single player mode
class AIPlayer(Player):
    def __init__(self, player):
        super(AIPlayer, self).__init__(player.ident, player.x, player.y, player.height, player.speed)

    # Make a move (always try to move closer to the other player)
    def getNextMove(self, targetPlayer):
        if targetPlayer.x < self.x:
            if self.lastDirection != LEFT:
                self.lastDirection = LEFT
                return LEFT
        if targetPlayer.x > self.x:
            if self.lastDirection != RIGHT:
                self.lastDirection = RIGHT
                return RIGHT
        if targetPlayer.y < self.y:
            if self.lastDirection != UP:
                self.lastDirection = UP
                return UP
        if targetPlayer.y > self.y:
            if self.lastDirection != DOWN:
                self.lastDirection = DOWN
                return DOWN

# Plotting class for displaying and saving streaming data from the devices
class HHPlot:
    def __init__(self, ser, maxLen):
        self.ser = ser
        self.ax = deque([0.0]*maxLen)
        self.ay = deque([0.0]*maxLen)
        self.az = deque([0.0]*maxLen)
        self.maxLen = maxLen
        self.csvfile = open('helping_hand_data.csv', 'wb')
        self.csvout = csv.writer(self.csvfile)

    def addToBuf(self, buf, val):
        if len(buf) < self.maxLen:
            buf.append(val)
        else:
            buf.pop()
            buf.appendleft(val)

    def add(self, data):
        assert(len(data) == 3)
        self.addToBuf(self.ax, data[0])
        self.addToBuf(self.ay, data[1])
        self.addToBuf(self.az, data[2])

        self.csvout.writerow(data)
        self.csvfile.flush()

    def update(self, frameNum, a0, a1, a2):
        try:
            line = self.ser.readline().strip()
            data = [ord(val) for val in line.split()]

            if(len(data) == 3):
                self.add(data)
                a0.set_data(range(self.maxLen), self.ax)
                a1.set_data(range(self.maxLen), self.ay)
                a2.set_data(range(self.maxLen), self.az)
        except KeyboardInterrupt:
            print('exiting')

        return a0,

# The Main control class for running the game
class HelpingHand(object):
    def __init__(self, num_obstacles=40):
        # Initialize PyGame state
        global FPSCLOCK, DISPLAYSURF
        pygame.init()
        FPSCLOCK = pygame.time.Clock()
        DISPLAYSURF = pygame.display.set_mode((WINDOWWIDTH, WINDOWHEIGHT))
        pygame.display.set_caption('Helping Hand')

        # Set up the XBee connection
        self.ser = serial.Serial('/dev/ttyAMA0', 9600, timeout=15)
        self.ser.open()

        # Create the menu
        menu = cMenu(50, 50, 20, 5, 'vertical', 100, DISPLAYSURF,
                   [('Start Rehabilitation (One Player)', 1, None),
                    ('Start Rehabilitation (Two Player)', 2, None),
                    ('Plot Live Measurements',         3, None),
                    ('Exit',                           4, None)])
        menu.set_center(True, True)
        menu.set_alignment('center', 'center')

        # Create the state variables (make them different so that the user event is
        # triggered at the start of the "while 1" loop so that the initial display
        # does not wait for user input)
        state = 0
        prev_state = 1

        # rect_list is the list of pygame.Rect's that will tell pygame where to
        # update the screen (there is no point in updating the entire screen if only
        # a small portion of it changed!)
        rect_list = []

        # Ignore mouse motion (greatly reduces resources when not needed)
        pygame.event.set_blocked(pygame.MOUSEMOTION)

        # Use the Menu library to build a simple Menu, the code below has been
        # adapted from the provided example Menu script
        while True:
            # Check if the state has changed, if it has, then post a user event to
            # the queue to force the menu to be shown at least once
            if prev_state != state:
                pygame.event.post(pygame.event.Event(EVENT_CHANGE_STATE, key = 0))
                prev_state = state

            # Get the next event
            e = pygame.event.wait()

            # Update the menu, based on which "state" we are in - When using the menu
            # in a more complex program, definitely make the states global variables
            # so that you can refer to them by a name
            if e.type == pygame.KEYDOWN or e.type == EVENT_CHANGE_STATE:
                if state == 0:
                    rect_list, state = menu.update(e, state)
                elif state == 1:
                    if self.ser is not None and self.ser.isOpen():
                            r = Response()
                            r.plot_mode = 0
                            print r.asByte
                            self.ser.write(array.array('B', [r.asByte]).tostring())
                    self.run_game(num_obstacles, 1)
                    state = 0
                elif state == 2:
                    if self.ser is not None and self.ser.isOpen():
                            r = Response()
                            r.plot_mode = 0
                            print r.asByte
                            self.ser.write(array.array('B', [r.asByte]).tostring())
                    self.run_game(num_obstacles, 2)
                    state = 0
                elif state == 3:
                    if self.ser is not None and self.ser.isOpen():
                        r = Response()
                        r.plot_mode = 1
                        print r.asByte
                        self.ser.write(array.array('B', [r.asByte]).tostring())
                    self.run_plotting()
                    state = 0
                else:
                    self.terminate()

            # Quit if the user presses the exit button
            if e is not None and e.type == pygame.QUIT:
                self.terminate()

            # Update the screen
            pygame.display.update(rect_list)

    # Run the plotting mode
    def run_plotting(self):
        hhplot = HHPlot(self.ser, 100)

        fig = plt.figure()
        ax = plt.axes(xlim=(0, 100), ylim=(0, 105))
        a0, = ax.plot([], [], label='Left Pressure')
        a1, = ax.plot([], [], label='Right Pressure')
        a2, = ax.plot([], [], label='Accelerometer')
        plt.legend((a0, a1, a2), ('Left Pressure', 'Right Pressure', 'Accelerometer'))
        anim = animation.FuncAnimation(fig, hhplot.update,
                                       fargs=(a0, a1, a2),
                                       interval=50)
        plt.show()

    # Run the game mode
    def run_game(self, num_obstacles, players):
        # Draw random obstacles
        self.obstacles = []
        for i in range(num_obstacles):
            self.obstacles.append(pygame.Rect(random.randint(0, WINDOWWIDTH), random.randint(0, WINDOWHEIGHT), random.randint(1, 5) * CELLSIZE, random.randint(1, 5) * CELLSIZE))
        self.drawObstacles()

        # Set up player(s)
        self.player1 = self.initializePlayer(1)
        if players == 2:
            self.player2 = self.initializePlayer(2)
        else:
            self.player2 = AIPlayer(self.initializePlayer(2))
        self.players = [self.player1, self.player2]
        self.drawPlayers()

        direction = RIGHT
        while True: # main game loop
            if self.ser is not None and self.ser.isOpen():
                # Process the command coming from the device and perform a move
                c = self.getCommand(self.ser.readline().strip())
                if c is not None:
                    dir = c.action
                    if c.left:
                        dir = 2
                    if c.right:
                        dir = 3
                    if c.hand == 0:
                        for i in range(c.speed):
                            self.player1.move(self.getDirection(dir), self.obstacles)
                    if isinstance(self.player2, AIPlayer):
                        self.player2.move(self.player2.getNextMove(self.player1), self.obstacles)
                    else:
                        if c.hand == 1:
                            for i in range(c.speed):
                                self.player2.move(self.getDirection(dir), self.obstacles)
                    if self.player1.colliderect(self.player2):
                        DISPLAYSURF.fill(RED)
                        time.sleep(3)
                        print 'GAME OVER'
                        return
            for event in pygame.event.get():
                if event.type == QUIT:
                    self.terminate()
                # Handle keyboard inputs as a backup/debug mode
                elif event.type == KEYDOWN:
                    if (event.key == K_LEFT or event.key == K_a):
                        direction = LEFT
                    elif (event.key == K_RIGHT or event.key == K_d):
                        direction = RIGHT
                    elif (event.key == K_UP or event.key == K_w):
                        direction = UP
                    elif (event.key == K_DOWN or event.key == K_s):
                        direction = DOWN
                    elif event.key == K_ESCAPE:
                        self.terminate()
                    self.player1.move(direction, self.obstacles)
                    if isinstance(self.player2, AIPlayer):
                        self.player2.move(self.player2.getNextMove(self.player1), self.obstacles)
                    else:
                        self.player2.move(direction, self.obstacles)
                    if self.player1.colliderect(self.player2):
                        DISPLAYSURF.fill(RED)
                        time.sleep(3)
                        print 'GAME OVER'
                        return
            DISPLAYSURF.fill(BGCOLOR)
            self.drawObstacles()
            self.drawPlayers()
            pygame.display.update()
            FPSCLOCK.tick(FPS)

    # Exit the game and clean up
    def terminate(self):
        if self.ser is not None and self.ser.isOpen():
            r = Response()
            r.exit = 1
            print r.asByte
            self.ser.write(array.array('B', [r.asByte]).tostring())
        pygame.quit()
        sys.exit()

    # Choose a random direction
    def getRandomDirection(self):
        r = random.randint(0, 3)
        self.getDirection(r)

    # Parse the direction from the set values
    def getDirection(self, r):
        if r == 0:
            return UP
        if r == 1:
            return DOWN
        if r == 2:
            return LEFT
        if r == 3:
            return RIGHT

    # Parse the command input coming from the devices
    def getCommand(self, input):
        c = Command()
        try:
            c.asByte = ord(input)
        except:
            return None
        if DEBUG:
            print ' ------ '
            print "hand:    %i" % c.hand
            print "action:  %i" % c.action
            print "speed:   %i" % c.speed
            print "left:    %i" % c.left
            print "right:   %i" % c.right
        return c

    # Parse the menu command input
    def getMenuCommand(self, input):
        c = MenuCommand()
        if DEBUG:
            print ' ---DEBUG--- '
            print input
        try:
            c.asByte = ord(input)
        except:
            return None
        if DEBUG:
            print ' ------ '
            print "hand:    %i" % c.hand
            print "up:      %i" % c.up
            print "down:    %i" % c.down
            print "select:  %i" % c.select
        return c

    # Draw the obstacle rectangles on the map
    def drawObstacles(self):
        for o in self.obstacles:
            pygame.draw.rect(DISPLAYSURF, DARKGREEN, o)

    # Draw the players on the map
    def drawPlayers(self):
        pygame.draw.rect(DISPLAYSURF, WHITE, self.player1)
        pygame.draw.rect(DISPLAYSURF, RED, self.player2)

    # Initialize the player object (ensure they're on the map and not overlapping
    # another player or an obstacle)
    def initializePlayer(self, ident):
        player = Player(ident, random.randint(0, WINDOWWIDTH), random.randint(0, WINDOWHEIGHT))

        # Find a location where the player object does not intersect with the obstacles
        while True:
            x = player.x
            y = player.y
            for obs in self.obstacles:
                if player.colliderect(obs):
                    player = Player(ident, random.randint(0, WINDOWWIDTH), random.randint(0, WINDOWHEIGHT))
            if player.x == x and player.y == y:
                break

        return player

# Start the game when run from command-line without parameters
if __name__ == '__main__':
    hh = HelpingHand(NUM_OBSTACLES)
