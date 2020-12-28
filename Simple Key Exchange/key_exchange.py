import sqlite3
import datetime
import requests
from bs4 import BeautifulSoup

key_db = '__HOME__/key_exchange/key_exchange.db'

def caesar_cipher(message, shift, encrypt):
    if not encrypt:
        shift = -1 * shift

    result = []
    for pos in message:
        result.append(chr((ord(pos) - 32 + shift) % 95 + 32))

    output = "".join(result)
    return output

def vigenere_cipher(message, keyword, encrypt):
    shifted = []
    for i in keyword:
        shifted.append(ord(i) - 32)

    result = ""
    for j in range(len(message)):
        new = caesar_cipher(message[j], shifted[j % len(keyword)], encrypt)
        result += new

    return result

def generate_keyword(key):
    alpha_string = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
    alpha_dict = {}
    index = 0
    keyword_array = [0,0,0,0,0,0]

    for char in alpha_string:
        alpha_dict.update({index:char})
        index += 1

    for i in range(0,6):
        index = (key + (i * 6)) % 37
        keyword_array[i] = alpha_dict[index]

    result = "".join(keyword_array)

    return result

def dhke(t, p, m, a, message_in, encrypt):
    key = (t**a) % p
    word = generate_keyword(key)
    return vigenere_cipher(message_in, word, encrypt)

def wiki_interfacer(topic):
    to_send = "https://en.wikipedia.org/w/api.php?titles={}&action=query&prop=extracts&redirects=1&format=json&exintro=".format(topic)
    r = requests.get(to_send)
    data = r.json()

    number = str(list(data["query"]["pages"].keys())[0])
    raw = data["query"]["pages"][number]["extract"]
    info = BeautifulSoup(raw, "html.parser")

    output = info.get_text()
    output = output.strip()
    output = output.replace("\n", " ")

    # l = len(output)
    # output = output[0:l]
    #
    # while output[l] != ".":
    #     output = output[0:l]
    #     if l == 0:
    #         return str(output)
    #
    #     l += -1

    return str(output)

def request_handler(request):
    if 'POST' in request['method']:
        if bool(request['values']) == True and bool(request['form']) == False:
            message = request['values']['query']
            p = int(request['values']['p'])
            m = int(request['values']['m'])
            t_mcu = int(request['values']['t_mcu'])
        elif bool(request['form']) == True and bool(request['values']) == False:
            message = request['form']['query']
            p = int(request['form']['p'])
            m = int(request['form']['m'])
            t_mcu = int(request['form']['t_mcu'])

        topic = str(dhke(t_mcu, p, m, 5, message, False))

        output = str(wiki_interfacer(topic))

        t_server = m*m*m*m*m % p

        result = str(dhke(t_server, p, m, 5, output, True))


        conn = sqlite3.connect(key_db)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()  # make cursor into database (allows us to execute commands)

        try:
            c.execute('''CREATE TABLE messages (ID INTEGER PRIMARY KEY, time timestamp, result text);''') # run a CREATE TABLE command

        except:
            c.execute('''INSERT into messages (time, result) VALUES (?,?);''', (datetime.datetime.now(), result))
            conn.commit() # commit commands
            conn.close() # close connection to database

    elif 'GET' in request['method']:
        conn = sqlite3.connect(key_db)  # connect to that database (will create if it doesn't already exist)
        c = conn.cursor()  # make cursor into database (allows us to execute commands)

        message = c.execute("SELECT * FROM messages ORDER BY time DESC").fetchone()
        conn.commit() # commit commands
        conn.close() # close connection to database

        return message[2]
