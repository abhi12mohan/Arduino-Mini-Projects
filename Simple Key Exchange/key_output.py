import sqlite3
import datetime

def request_handler(request):
    p = 30
    m = 16
    t_server = (m * m * m * m * m) % p

    output = "p=%i,m=%i,t_server=%i," % (p,m,t_server)
    return output
