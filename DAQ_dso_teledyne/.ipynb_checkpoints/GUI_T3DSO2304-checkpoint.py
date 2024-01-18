#new version with thread

import pyvisa as vi
import threading
import time
import PySimpleGUI as sg
import numpy as np
import matplotlib
#import matplotlib.pyplot as plt 
import numpy as np
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, FigureCanvasAgg
from matplotlib.figure import Figure
import pandas as pd
import numpy as np
from datetime import datetime

version = '20240103'



########definitions

############################################################################
def init_com():
    print("func: init_com()")
    rm = vi.ResourceManager()
    print(rm.list_resources())
    dso = rm.open_resource("USB0::0xF4EC::0xEE3A::T0101C20360610::INSTR")
    print(dso.query("*IDN?"))
    return dso

############################################################################
def close_com():
    print("func: close_com()")

############################################################################
def config_meas(dso):
    print("func: config_meas()")
    dso.write("TRMD SINGLE")
    dso.timeout = 5000 #default value is 2000(2s)
    dso.chunk_size = 20*1024#*1024 #default value is 20*1024(20k bytes)
    
############################################################################
def start_meas(dso, CHA):
   #CHA array di canali 

    tdiv_strg = dso.query("tdiv?")
    sara_strg = dso.query("sara?")
    DSOCONFIG = tdiv_strg+'\n'+sara_strg+'\n'
    tdiv = float(tdiv_strg.replace('TDIV ', '').replace('S\n', ''))
    sara = float(sara_strg.replace('SARA ', '').replace('GSa/s\n', 'e+09').replace('MSa/s\n', 'e+06').replace('kSa/s\n', 'e+03'))
        
    dso.write("ARM")
    
    print('waiting data: ')
    
    dic={}
    while True and DAQ: #entra in un loop eterno che interrompe se il dso ha triggerato o DAQ è falso
        x = dso.query("SAST?")
        print('', end='*')
        if x == 'SATA Stop\n':
            time.sleep(0.1)
            for ch in CHA:
                vdiv_strg = dso.query(f"c{ch}:vdiv?")
                ofst_strg = dso.query(f"c{ch}:ofst?")
                DSOCONFIG += vdiv_strg+'\n'+ofst_strg+'\n'
                vdiv = float(vdiv_strg.replace(f'C{ch}:VDIV ', '').replace('V\n', ''))
                ofst = float(ofst_strg.replace(f'C{ch}:OFST ', '').replace('V\n', ''))
                #time.sleep(0.1)
                dso.write(f"c{ch}:wf? dat2")
                raw = dso.read_raw()
                recv = list(raw)
                recv = np.array(recv)
                datac = (recv>127)*(recv-255) + (recv<=127)*recv 
                datac = datac[22:-2]
                time_value = []
                volt_value = []
                for idx in range(0,len(datac)):
                    volt_value.append(datac[idx]/25*vdiv-ofst)
                    time_value.append(idx*(1/sara) - tdiv*14/2)
                dic['DSOCONFIG']=DSOCONFIG
                dic[f'X']=time_value
                dic[f'C{ch}']=volt_value
            
            break
        else: time.sleep(0.1)
    
    print('\ncollected data: ', dic.keys())
    return dic

############################################################################
def draw_figure(canvas, figure, loc=(0, 0)):
    print("func: draw_figure()")
    figure_canvas_agg = FigureCanvasTkAgg(figure, canvas)
    figure_canvas_agg.draw()
    figure_canvas_agg.get_tk_widget().pack(side='top', fill='both', expand=1)
    return figure_canvas_agg

def long_operation_thread(window, data, dso):
    print("func: long_operation_thread()")
    tempdic = start_meas(dso, CHA)
    data.clear()
    data.update(tempdic)
    if DAQ: window.write_event_value('acquisition', True)  # put a message into queue for GUI
    #return data

    
            
############################################## main program ##################################################

strumenti=['1','2', '3', '4' ]
#########layout
sg.theme('LightBlue')
layout = [
    [sg.Text('Live DAQ Oscilloscope '+version)],
    [sg.Button('Start Communication')],
    [sg.Checkbox('C1:', default=True, key="-C1-"),sg.Checkbox('C2:', default=False, key="-C2-"),sg.Checkbox('C3:', default=False, key="-C3-"),sg.Checkbox('C4:', default=False, key="-C4-")],
    #[sg.Text('')],
    [sg.Checkbox('Save waveform?:', default=False, key="-SAVE-"), sg.Text('Path'), sg.Input(key='-PATH-', default_text='D:/Dati_DSO/FCC/nuovo.txt', size=(100, 1))],
    [sg.Button('Start DAQ', bind_return_key=True), sg.Button('Stop DAQ'), sg.Button('Clear')],
    [sg.Canvas(size=(640*2/4, 480*2/4), key='-CANVAS0-'), sg.Canvas(size=(640*2/4, 480*2/4), key='-CANVAS1-')],
    [sg.Text('BINS'), sg.Input(key='-BINS-', default_text='10')],
    #[sg.Button('Clear')], 
    [sg.Button('Exit')],
    [sg.Text('Debug display')],
    [sg.Output(size=(100, 10))]]
    

    #[sg.Text('Select Channel'), sg.Combo(list(strumenti), size=(20,4), enable_events=False, key='-INSTR-'), sg.Button('Start Communication')],

window = sg.Window('Oscilloscope', layout, background_color='#FFFFFF', finalize=True)

#########Plots

fig0 = Figure()
ax0 = fig0.add_subplot(111)
ax0.set_xlabel("Time [s]")
ax0.set_ylabel("Amplitude [V]")
ax0.set_title("Waveforms")
ax0.grid()
fig_agg0=draw_figure(window["-CANVAS0-"].TKCanvas, fig0)

fig1 = Figure()
ax1 = fig1.add_subplot(111)
ax1.set_xlabel("Amplitude [V]")
ax1.set_ylabel("Counts")
ax1.grid()
ax1.set_title("Amplitude histogram")
fig_agg1=draw_figure(window["-CANVAS1-"].TKCanvas, fig1)

########Vars
th1d=[]
counter=0
DAQ=False
#window.write_event_value('acquisition', False)  # put a message into queue for GUI
data={}
INIT=False
HEADERSAVED = False



########Event loop: macchina a stati della gui
while True:
    
    event, values = window.read()
    
#------------------------------------------------------------    
    if event in (sg.WIN_CLOSED, 'Exit'):
        close_com()
        break
        
#------------------------------------------------------------    
    elif event == 'Start Communication':
        print('\nCase: -Starting Communication-')
        DAQ=False
        dso = init_com()
        INIT = True
        
#------------------------------------------------------------    
    elif event == 'Start DAQ':
        print('\nCase: -Starting DAQ-')
        if INIT: 
            CHA=[]
            if values['-C1-']==True:
                CHA+=[1]
            if values['-C2-']==True:
                CHA+=[2]
            if values['-C3-']==True:
                CHA+=[3]
            if values['-C4-']==True:
                CHA+=[4]
            DAQ=True
            HEADERSAVED = False
            measconfig=config_meas(dso)
            window.write_event_value('acquisition', False)  # put a message into queue for GUI
        else: print('INITIALIZE!!!!!')
#------------------------------------------------------------    
    elif event == 'acquisition':
        print('\nCase: -acquisition-')

        if values[event] == False:
            if DAQ==True:
                print('treading')
                threading.Thread(target=long_operation_thread, args=(window, data, dso), daemon=True).start()
            
        if values[event] == True:
            print('plotting')
            ax0.cla()
            ax0.grid()
            KE = list(data.keys())
            KE.remove('X')
            KE.remove('DSOCONFIG')
            X=data['X']
            for k in KE:
                ax0.plot(X, data[k], label=k, color=k)
                if k == 'C1': th1d+=[np.max(np.abs(data[k]))]

            ax0.legend()
            ax0.set_xlabel("Time [s]")
            ax0.set_ylabel("Amplitude [V]")
            ax0.set_title("Waveforms")
            fig_agg0.draw()
            
            ax1.cla()
            ax1.grid()
            ax1.set_ylabel("Counts")
            ax1.set_xlabel("Amplitude [V]")
            ax1.set_title("Amplitude histogram C1")
            ax1.hist(th1d, bins=int(values['-BINS-']), color='gray')
            ax1.annotate(f'counts: {len(th1d)}\nmean: {np.mean(th1d)}\nstd: {np.std(th1d)}',
                        xy=(0.98, 0.98), xycoords='axes fraction',
                        horizontalalignment='right', verticalalignment='top')
            fig_agg1.draw()
            #time.sleep(1)                  # sleep for a while

            #print('Saved data at: {}'.format(spath))
            counter=counter+1
            
            if values['-SAVE-'] == True:
                print('saving')
                path = values['-PATH-']
                C=np.ones_like(X)*counter
                df = pd.DataFrame(C, columns=['frame'])
                df['Time_Stamp'] = time.time()
                df['Time'] = X
                for k in KE:
                    df[k] = data[k]

                if HEADERSAVED==False:
                    current_datetime = datetime.now()
                    formatted_datetime = current_datetime.strftime("%Y-%m-%d %H:%M:%S\n\n")
                    file = open(path, 'w')
                    file.write(formatted_datetime)
                    file.write(data['DSOCONFIG'])
                    file.close()
                    df.to_csv(path, index=None, sep=' ', mode='a', header=True)
                    HEADERSAVED = True
                else:
                    df.to_csv(path, index=None, sep=' ', mode='a', header=False)

                
            window.write_event_value('acquisition', False)  # put a message into queue for GUI

#------------------------------------------------------------    
    elif event == 'Stop DAQ':
        print('\nCase: -Stopped DAQ-')
        DAQ=False
        
#------------------------------------------------------------    
    elif event == 'Clear':
        print('\nCase: -Clear Plots-')
        th1d=[]
        counter=0
        #DAQ=False
        ax1.cla()
        ax1.set_xlabel("Amplitude [V]")
        ax1.set_ylabel("Counts")
        ax1.grid()
        ax1.set_title("Amplitude histogram")
        fig_agg1.draw()
        ax0.cla()
        ax0.grid()
        ax0.set_xlabel("Time [s]")
        ax0.set_ylabel("Amplitude [V]")
        ax0.set_title("Waveforms")
        fig_agg0.draw()
        

window.close()

print('event: ', event)
print('values: ', values)