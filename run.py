from mpl_toolkits.mplot3d import Axes3D
from time import sleep
from datetime import datetime
import serial
import matplotlib.pyplot as plt
import pickle
import csv, os, sys


ser = serial.Serial('/dev/ttyACM2', baudrate=115200, timeout=1)
ser.flushInput()

def main(argv):
    try:
        if len(argv) < 1:
            sys.exit("Incorrect usage. Expected: run.py manual or run.py auto <filename>")
        if str(argv[0]) == "manual":
            flag = False
            while flag == False:
                line = ser.readline()[:-2]
                if line:
                    if line == b'CONNECTED':
                        ser.flushInput()
                        flag = True
            manual_mode()

        elif str(argv[0]) == "auto":
            if len(argv) != 2:
                sys.exit("Incorrect usage. Expected: run.py manual or run.py auto <filename>")
            elif (os.path.isfile(str(argv[1]))):
                auto(str(argv[1]))
            else:
                sys.exit("File not found")
    except KeyboardInterrupt:
        os.system('clear')
        change_mode("OFF")
        sys.exit("Program Terminated")

def manual_mode():
    change_mode("MANUAL")
    
    while(True):
        command = "GET_RESULTS"
        ser.write(command.encode("ascii"))
        line = ser.readline()[:-2]
        sleep(0.5)
        line = line.decode("ascii")
        if line:
            val_list = line.split(', ')
            values = map(float, val_list)
            field_list = list(values)
            X_mag = field_list[0]
            Y_mag = field_list[1]
            Z_mag = field_list[2]
            os.system('clear')
            print("X field:", X_mag)
            print("Y field:", Y_mag) 
            print("Z_field", Z_mag)
            

def get_pos():
    command = "GET_POS"
    ser.write(command.encode("ascii"))
    while True:
        line = ser.readline()[:-2]
        if line:
            val_list = line.split(', ')
            values = map(float, val_list)
            field_list = list(map)
            X = field_list[0]
            Y = field_list[1]
            return X, Y

def get_bounds():
    command = "GET_BOUNDS"
    ser.write(command.encode("ascii"))
    while True:
        line = ser.readline()[:-2]
        line = line.decode("ascii")
        if line:
            val_list = line.split(', ')
            values = map(float, val_list)
            field_list = list(values)
            x_min = field_list[0]
            x_max = field_list[1]
            y_min = field_list[2]
            y_max = field_list[3]

            return x_min, x_max, y_min, y_max

def move_to(x, y):
    flag = False
    command = "GO_TO" +   " " + str(x) +   " " + str(y)
    ser.write(command.encode("ascii"))
    while flag == False:
        line = ser.readline()[:-2]
        if line:
            if line == b'DONE':
                flag = True
        

def calibrate():
    os.system('clear')
    print("Please calibrate. Use joystick to move sensor to one corner of test area. Press joystick to select")
    command = "CALIBRATE"
    ser.write(command.encode("ascii"))
    flag = False
    while flag == False:
        line = ser.readline()[:-2]
        if line:
            if line == b'MIN_SET':
                print("First corner point set. Move to opposite corner and press joystick")
                flag = True
    flag = False
    while flag == False:
        line = ser.readline()[:-2]
        if line:
            if line == b'MAX_SET':
                print("Second corner point set.")
                flag = True
    flag = False
    while flag == False:
        line = ser.readline()[:-2]
        if line:
            if line == b'DONE':
                flag = True


def get_calibrated():
    command = "GET_CALIBRATED"
    ser.write(command.encode("ascii"))
    while True:
        line = ser.readline()[:-2]
        if line:
            if line == b'TRUE':
                return True
            else:
                return False

def auto(path_string):
    if get_calibrated() == False:
        calibrate()
    change_mode("AUTO")
    path = read_file_to_path(path_string)
    x_min, x_max, y_min, y_max = get_bounds()
    x_range = abs(x_max - x_min)
    y_range = abs(y_max - y_min)
    X_mag, Y_mag, Z_mag = [], [], []
    X, Y, iters = [], [], []
    i = 0
    for x, y in path:
        move_to(int(x_min + x_range * x), int(y_min + y_range * y))
        flag = False
        command = "GET_RESULTS"
        ser.write(command.encode("ascii"))
        while flag == False:
            line = ser.readline()[:-2]
            line = line.decode("ascii")
            if line:
                flag = True
                val_list = line.split(', ')
                values = map(float, val_list)
                field_list = list(values)
                X_mag.append(field_list[0])
                Y_mag.append(field_list[1])
                Z_mag.append(field_list[2])
                X.append(int(x_range * x))
                Y.append(int(y_range * y))
                iters.append(i)
                i += 1
        sleep(0.1)

    x_save_name = 'magnetic_field_x_' + datetime.now().strftime("%Y%m%d-%H%M%S") + '.png'
    y_save_name = 'magnetic_field_y_' + datetime.now().strftime("%Y%m%d-%H%M%S") + '.png'
    save_name = 'magnetic_field_3D_' + datetime.now().strftime("%Y%m%d-%H%M%S") + '.png'
    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.scatter(X, X_mag, label='X_mag')
    ax.scatter(X, Y_mag, label='Y_mag')
    ax.scatter(X, Z_mag, label='Z_mag')
    plt.xlabel('X Position')
    plt.ylabel('Magnetic Field')
    plt.legend(loc='best')
    plt.savefig(x_save_name)

    fig = plt.figure()
    ax = fig.add_subplot(111)
    ax.scatter(Y, X_mag, label='X_mag')
    ax.scatter(Y, Y_mag, label='Y_mag')
    ax.scatter(Y, Z_mag, label='Z_mag')
    plt.xlabel('Y Position')
    plt.ylabel('Magnetic Field')
    plt.legend(loc='best')
    plt.savefig(y_save_name)

    fig = plt.figure()
    ax = fig.add_subplot(111, projection='3d')
    ax.scatter(X, Y, X_mag, label='X_mag')
    ax.scatter(X, Y, Y_mag, label='Y_mag')
    ax.scatter(X, Y, Z_mag, label='Z_mag')

    ax.set_xlabel('X Position')
    ax.set_ylabel('Y Position')
    ax.set_zlabel('Magnetic Field')
    plt.legend(loc='best')
    plt.savefig(save_name)

    file_save_name = 'data_' + datetime.now().strftime("%Y%m%d-%H%M%S") + '.csv'
    rows = zip(iters, X, Y, X_mag, Y_mag, Z_mag)
    with open(file_save_name, "w") as f:
        writer = csv.writer(f)
        for row in rows:
            writer.writerow(row)

    print("Finished measurement. Entering manual mode.")
    manual_mode()

def change_mode(mode):
    command = "CHANGE_MODE" + " " + mode
    flag = False
    count = 0
    ser.flushInput()
    ser.flushOutput()
    ser.write(command.encode("ascii"))
    while flag == False:
        line = ser.readline()[:-2]
        if line:
            if line == b'DONE':
                flag = True

def write_path_to_file(path, filename):
    with open(filename, "wb") as f:
        pickle.dump(path, f)

def read_file_to_path(filename):
    with open(filename, "rb") as f:
        path = pickle.load(f)
    return path

if __name__ == "__main__":
   main(sys.argv[1:])