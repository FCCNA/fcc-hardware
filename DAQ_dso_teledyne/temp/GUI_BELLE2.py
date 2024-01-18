
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


########definitions

############################################################################
def init_com(ch):
    print("func: init_com()")
    print(f'Channel: {ch}')
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
   #CHA array di canali definiti su init comunication
    tdiv_strg = dso.query("tdiv?")
    sara_strg = dso.query("sara?")
    tdiv = float(tdiv_strg.replace('TDIV ', '').replace('S\n', ''))
    sara = float(sara_strg.replace('SARA ', '').replace('GSa/s\n', 'e+09').replace('MSa/s\n', 'e+06').replace('kSa/s\n', 'e+03'))
        
    dso.write("ARM")

    while True:
        x = dso.query("SAST?")
        print(x)
        if x == 'SATA Stop\n':
            break
        else: time.sleep(0.1)

    time.sleep(0.1)
    dic={}
        
    for ch in CHA:
        vdiv_strg = dso.query(f"c{ch}:vdiv?")
        ofst_strg = dso.query(f"c{ch}:ofst?")
        vdiv = float(vdiv_strg.replace(f'C{ch}:VDIV ', '').replace('V\n', ''))
        ofst = float(ofst_strg.replace(f'C{ch}:OFST ', '').replace('V\n', ''))
        time.sleep(0.1)

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
            
        dic[f'X{ch}']=time_value
        dic[f'Y{ch}']=volt_value
    
    print(dic.keys())

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
    data.update(tempdic)
    window.write_event_value('Datavailable', True)  # put a message into queue for GUI
    #return data
    
    
    
    
    
            
############################################## main program ##################################################

strumenti=['1','2', '3', '4' ]
#########layout
sg.theme('LightBlue')
layout = [
    [sg.Text('Live DAQ Oscilloscope')],
    [sg.Checkbox('C1:', default=True, key="-C1-"),sg.Checkbox('C2:', default=False, key="-C2-"),sg.Checkbox('C3:', default=False, key="-C3-"),sg.Checkbox('C4:', default=False, key="-C4-"), sg.Button('Start Communication')],
    [sg.Text('')],
    [sg.Checkbox('Save waveform?:', default=False, key="-SAVE-"), sg.Text('Path'), sg.Input(key='-PATH-', default_text='D:/Dati_dso/nuovo.txt', size=(40, 1))],
    [sg.Button('Start DAQ', bind_return_key=True), sg.Button('Stop DAQ'), sg.Button('Clear')],
    [sg.Canvas(size=(640*2/4, 480*2/4), key='-CANVAS0-'), sg.Canvas(size=(640*2/4, 480*2/4), key='-CANVAS1-')],
    #[sg.Button('Clear')], 
    [sg.Button('Exit')],
    [sg.Text('Debug display')],
    [sg.Output(size=(100, 5))],
        ]

    #[sg.Text('Select Channel'), sg.Combo(list(strumenti), size=(20,4), enable_events=False, key='-INSTR-'), sg.Button('Start Communication')],

window = sg.Window('Oscilloscope', layout, background_color='#FFFFFF', finalize=True)

#########Plots
fig1 = Figure()
ax1 = fig1.add_subplot(111)
ax1.set_xlabel("Amplitude [V]")
ax1.set_ylabel("Counts")
ax1.grid()
ax1.set_title("Amplitude histogram")
fig_agg1=draw_figure(window["-CANVAS1-"].TKCanvas, fig1)

fig0 = Figure()
ax0 = fig0.add_subplot(111)
ax0.set_xlabel("Time [s]")
ax0.set_ylabel("Amplitude [V]")
ax0.grid()
fig_agg0=draw_figure(window["-CANVAS0-"].TKCanvas, fig0)

########Vars
th1d=[]
counter=0
DAQ=False
window.write_event_value('Datavailable', False)  # put a message into queue for GUI
data={}






########Event loop:
while True:
    
    event, values = window.read()
    
#------------------------------------------------------------    
    if event in (sg.WIN_CLOSED, 'Exit'):
        close_com()
        break
        
#------------------------------------------------------------    
    elif event == 'Start Communication':
        print('Case: -Starting Communication-')
        CHA=[]
        if values['-C1-']==True:
            CHA+=[1]
        if values['-C2-']==True:
            CHA+=[2]
        if values['-C3-']==True:
            CHA+=[3]
        if values['-C4-']==True:
            CHA+=[4]
        DAQ=False
        dso = init_com(CHA)
        
#------------------------------------------------------------    
    elif event == 'Start DAQ':
        print('Case: -Starting DAQ-')
        DAQ=True
        measconfig=config_meas(dso)
        window.write_event_value('Datavailable', False)  # put a message into queue for GUI

#------------------------------------------------------------    
    elif event == 'Datavailable':
        if values[event] == False:
            if DAQ==True:
                print('treading')
                threading.Thread(target=long_operation_thread, args=(window, data, dso), daemon=True).start()
                #data=long_operation_thread(window, data)
            
        if values[event] == True:
            ax0.cla()
            ax0.grid()
            
            print(data.keys())
            if 'Y1' in data.keys():
                Y1=data['Y1']
                X1=data['X1']
                th1d+=[np.max(np.abs(Y1))]
                ax0.plot(X1, Y1, label='Y1', color='C0')
            if 'Y2' in data.keys():
                Y2=data['Y2']
                X2=data['X2']
                ax0.plot(X2, Y2, label='Y2', color='C1')
            if 'Y3' in data.keys():
                Y3=data['Y3']
                X3=data['X3']
                ax0.plot(X3, Y3, label='Y3', color='C2')
            if 'Y4' in data.keys():
                Y4=data['Y4']
                X4=data['X4']
                ax0.plot(X4, Y4, label='Y4', color='C3')
            ax0.legend()
            ax0.set_xlabel("Time [s]")
            ax0.set_ylabel("Amplitude [V]")
            ax0.set_title("Waveform")
            fig_agg0.draw()
            ax1.cla()
            ax1.grid()
            ax1.set_ylabel("Counts")
            ax1.set_xlabel("Amplitude [V]")
            ax1.set_title("Amplitude histogram C1")
            ax1.hist(th1d, color='gray')
            ax1.annotate(f'counts: {len(th1d)}\nmean: {np.mean(th1d)}\nstd: {np.std(th1d)}',
                        xy=(0.98, 0.98), xycoords='axes fraction',
                        horizontalalignment='right', verticalalignment='top')
            fig_agg1.draw()
            #time.sleep(1)                  # sleep for a while

            #print('Saved data at: {}'.format(spath))
            
            counter=counter+1
            if values['-SAVE-'] == True:
                path = values['-PATH-']
                C=np.ones_like(X1)*counter
                df = pd.DataFrame(C, columns=['Windex'])
                
                if 'Y1' in data.keys():
                    df['Time1'] = X1
                    df['C1'] = Y1

                if 'Y2' in data.keys():
                    df['Time2'] = X2
                    df['C2'] = Y2

                if 'Y3' in data.keys():
                    df['Time3'] = X3
                    df['C3'] = Y3
        
                if 'Y4' in data.keys():
                    df['Time4'] = X4
                    df['C4'] = Y4

                df.to_csv(path, index=None, sep=' ', mode='a', header=False)    
                
            window.write_event_value('Datavailable', False)  # put a message into queue for GUI

#------------------------------------------------------------    
    elif event == 'Stop DAQ':
        print('Case: -Stopped DAQ-')
        DAQ=False
        
#------------------------------------------------------------    
    elif event == 'Clear':
        print('Case: -Clear Plots-')
        th1d=[]
        counter=0
        DAQ=False
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
        ax0.set_title("Waveform")
        fig_agg0.draw()
        
#------------------------------------------------------------    
    #elif event == '-DAQTHREAD-':
    #    2+2
        #print('Data recived?: ', values[event])

# if user exits the window, then close the window and exit the GUI func

    #break

window.close()

print(event)
print(values)