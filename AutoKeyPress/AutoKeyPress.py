import keyboard
import pyautogui
import pygame
import time
import threading
#pyinstaller your_script.py
speed = 1/15
key = '-'
obj = '`'
open_v = False
exit_flag = 0
exit_flag2 = threading.Event()
child_thread = None
x = 1191  # x-coordinate of the pixel
y = 985  # y-coordinate of the pixel


def play_sound_start():
    sound_file = './start.wav'
    pygame.mixer.music.load(sound_file)
    pygame.mixer.music.set_volume(1.0)  # Set volume level to maximum (1.0)
    
    pygame.mixer.music.play()

def play_sound_finish():
    sound_file = './finish.wav'
    pygame.mixer.music.load(sound_file)
    pygame.mixer.music.set_volume(1.0)  # Set volume level to maximum (1.0)
    
    pygame.mixer.music.play()

def simulate_key_press():
    global key,exit_flag,x,y
    while not exit_flag:
        pyautogui.moveTo(x, y)
        time.sleep(0.0001)
        pyautogui.click()
        time.sleep(speed)

def catch_input():
    global exit_flag2
    while not exit_flag2.is_set():
        user_input = input("")
        if user_input == 'exit':
            exit_flag2.set()
            break
        elif "speed=" in user_input:
            remaining = user_input[user_input.index('speed=') + len('speed='):].strip()

            try:
                speed = float(1/int(remaining))
                print("按键速度更改成功：%f" % speed)
            except ValueError:
                print("格式错误，如需更改默认格式请输入： speed=?")
        else:
            print("格式错误")

def on_key_press(event):
    global open_v, exit_flag,child_thread,obj
    if event.name == obj:
        if not open_v and not exit_flag:
            print("按键开启")
            play_sound_start()
            open_v = True
            child_thread = threading.Thread(target=simulate_key_press)
            exit_flag = 0
            child_thread.start()

        elif open_v and not exit_flag:
            print("按键关闭")
            play_sound_finish()
            open_v = False
            exit_flag = 1
            if (child_thread != None):
                child_thread.join()

            exit_flag = 0



pygame.mixer.init()
print("===========================================================")
print("欢迎来到Lucycycy设计的自动按键")
print("默认按键速度：15 (speed)")
print("默认自动按键：F9 (key)(暂不支持更改)")
print("默认按键宏：~ (object)(暂不支持更改)" )
print("如需更改默认格式请输入： speed=?")
print("退出输入：exit")
print("===========================================================")
child_thread2 = threading.Thread(target=catch_input)

child_thread2.start()
keyboard.on_press(on_key_press)



child_thread2.join()
print("成功退出")
