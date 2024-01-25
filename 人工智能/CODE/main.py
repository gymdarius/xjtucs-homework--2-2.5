from time import time
from BFS_search import breadth_first_search
from Astar_search import Astar_search
from puzzle import Puzzle


state=[0,1,2,7,8,3,6,5,4]


Puzzle.num_of_instances=0
t0=time()
bfs=breadth_first_search(state)
t1=time()-t0
print('BFS:', bfs)
print('price:', Puzzle.num_of_instances)
print('time:', t1)
print()

Puzzle.num_of_instances = 0
t2 = time()
astar = Astar_search(state)
t3 = time() - t0
print('A*:', astar)
print('price:', Puzzle.num_of_instances)
print('time:', t3)
print()
