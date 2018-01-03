from random import shuffle, randrange
import random
import sys
import time
import json

#Only useful for debugging
g_DebugStep = 0


def print_maze_string(hor, ver, x, y, grid):
    global g_DebugStep
    s = ""
    for (a, b) in zip(hor, ver):
        s += ''.join(a + ['\n'] + b + ['\n'])
    g_DebugStep = g_DebugStep + 1
    print("***********")
    print("step is: " + str(g_DebugStep))
    print("expanding " + str(x) + " , " + str(y))
    print(s)
    print("\n")
    print("grid is")
    s2 = ""
    for row in grid:
        #python string conversion = :(
        s2 += ",".join(map(str, row))
        s2 += "\n"
    print(s2)
    print("\n")
    return s


# Top closed 1
# left closed 2
# right closed 4
# bot closed 8
# debug printing based on https://rosettacode.org/wiki/Maze_generation#Python
def make_maze(w=16, h=8):
    vis = [[0] * w + [1] for _ in range(h)] + [[1] * (w + 1)]
    ver = [["|  "] * w + ['|'] for _ in range(h)] + [[]]
    hor = [["+--"] * w + ['+'] for _ in range(h + 1)]

    kTopFlag = 1 << 0 # 1
    kLeftFlag = 1 << 1  # 2
    kBotFlag = 1 << 2   # 4
    kRightFlag = 1 << 3 # 8
    kAllFlags = kTopFlag | kLeftFlag | kRightFlag | kBotFlag

    grid = [[kAllFlags]*w for i in range(h)]

    def walk(x, y):
        vis[y][x] = 1

        d = [(x - 1, y), (x, y + 1), (x + 1, y), (x, y - 1)]
        shuffle(d)
        for (xx, yy) in d:
            if vis[yy][xx]:
                continue
            if xx == x:
                hor[max(y, yy)][x] = "+  "
                if y < yy:
                    grid[y][x] = grid[y][x] & ~kBotFlag
                    grid[yy][x] = grid[yy][x] & ~kTopFlag
                elif y > yy:
                    grid[y][x] = grid[y][x] & ~kTopFlag
                    grid[yy][x] = grid[yy][x] & ~kBotFlag
            if yy == y:
                ver[y][max(x, xx)] = "   "
                if x < xx:
                    grid[y][x] = grid[y][x] & ~kRightFlag
                    grid[y][xx] = grid[y][xx] & ~kLeftFlag
                elif x > xx:
                    grid[y][x] = grid[y][x] & ~kLeftFlag
                    grid[y][xx] = grid[y][xx] & ~kRightFlag
            print_maze_string(hor,ver,x,y,grid)
            walk(xx, yy)

    walk(randrange(w), randrange(h))

    s = ""
    for (a, b) in zip(hor, ver):
        s += ''.join(a + ['\n'] + b + ['\n'])
    return s, grid

if __name__ == '__main__':
    #lazy args, width, height, seed.
    num_args = len(sys.argv)
    maze_width = 7
    maze_height = 5
    rand_seed = int(time.time())
    # this is a really simple project, so not bothering with arg parser
    # until I get better design specs.
    print sys.argv
    if num_args > 1:
        maze_width = int(sys.argv[1])
    if num_args > 2:
        maze_height = int(sys.argv[2])
    if num_args > 3:
        rand_seed = int(sys.argv[3])
    random.seed(rand_seed)

    maze_data = {}
    maze_data["rand_seed"] = rand_seed
    maze_data["maze_width"] = maze_width
    maze_data["maze_height"] = maze_height

    debug_string, grid = make_maze(maze_width, maze_height)
    maze_data["grid"] = grid
    maze_data["entryx"] = 0
    maze_data["entryy"] = 0
    maze_data["exitx"] = maze_width - 1
    maze_data["exity"] = maze_height - 1
    maze_data["version"] = 0

    print(debug_string)
    json1 = json.dumps(maze_data, ensure_ascii=True, sort_keys=True, indent=2)
    #print(json1)

    with open("mazedata.json", "w") as f:
        f.write(json1)

