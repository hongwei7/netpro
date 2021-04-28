import requests
import _thread as thread
from tqdm import tqdm

url = 'http://127.0.0.1:9999/'
k = 100

def tryPost(logProcess = True):
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
    print(success, 'success')

if __name__ == '__main__':
    print("需要创建多少线程:")
    n = int(input())
    k = 30
    show = False
    for i in range(n):
        try:
            thread.start_new_thread(tryPost, (show,))
        except:
            print("unable to create threads.")
    while 1:
        pass

