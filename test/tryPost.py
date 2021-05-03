import requests
from threading import Thread
from threading import Lock
from tqdm import tqdm

url = 'http://127.0.0.1:9999/'
k = 100
successAll = 0
lock = Lock()

def tryPost(logProcess = True):
    global successAll
    success = 0
    d = {'op': 'signin', 'username': 'hongwei711q', 'password': '12356789'}
    p = range(k)
    if(logProcess):
        p = tqdm(range(k))
    for i in p:
        try:
            r = requests.post(url, data=d)
            success = success + 1
        except:
            pass
    #print(success, 'success')
    lock.acquire()
    successAll += success
    lock.release()

if __name__ == '__main__':
    print("需要创建多少线程:")
    n = int(input())
    k = 30
    show = False
    queue = []
    for i in range(n):
        try:
            queue.append(Thread(target = tryPost, args = (show,)))
            queue[-1].start()
        except:
            print("unable to create threads.")
    for ti in queue:
        ti.join()
    print('succeed:', successAll,'/', k * n)
