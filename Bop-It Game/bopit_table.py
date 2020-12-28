import sqlite3
import datetime

stats_db = '__HOME__/bopit/bopit.db'

def request_handler(request):
    method = request['method']
    args = request['args']

    if 'POST' in method:
        if 'values' in request and bool(request['form']) == False:
            kerberos = request['values']['kerberos']
            score = int(request['values']['score'])
        elif 'form' in request and bool(request['values']) == False:
            kerberos = request['form']['kerberos']
            score = int(request['form']['score'])

        conn = sqlite3.connect(stats_db)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()  # make cursor into database (allows us to execute commands)
        try:
            c.execute('''CREATE TABLE bopit (ID INTEGER PRIMARY KEY, time timestamp, kerberos text, score int);''') # run a CREATE TABLE command

        except:
            c.execute('''INSERT into bopit (time, kerberos, score) VALUES (?,?,?);''', (datetime.datetime.now(), kerberos, score))
            conn.commit() # commit commands
            conn.close() # close connection to database

    if 'GET' in method:
        kerberos = request['values']['kerberos']

        conn = sqlite3.connect(stats_db)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()  # make cursor into database (allows us to execute commands)

        bopit_table = c.execute("SELECT * FROM bopit ORDER BY score DESC").fetchmany(2)
        conn.commit() # commit commands
        conn.close() # close connection to database

        output = ""
        for j in bopit_table:
            output += "Kerberos: " + str(j[2]) + "\nScore: " + str(j[3]) + "\nTime: " + str(j[1][:16]) +"\n\n"
        return output
