import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from termcolor import colored
import argparse
from __init__ import *
import os

plt.rcParams.update({
          'font.size': 20,
          'figure.figsize': (16, 9),
          'axes.grid': False,
          'grid.linestyle': '-',
          'grid.alpha': 0.2,
          'lines.markersize': 5.0,
          'xtick.minor.visible': True,
          'xtick.direction': 'in',
          'xtick.major.size': 10.0,
          'xtick.minor.size': 5.0,
          'xtick.top': True,
          'ytick.minor.visible': True,
          'ytick.direction': 'in',
          'ytick.major.size': 10.0,
          'ytick.minor.size': 5.0,
          'ytick.right': True,
          'errorbar.capsize': 0.0,
          'figure.max_open_warning': 50,
})

import yaml


def argparser():
    """
    Parse options as command-line arguments.
    """

    # Re-use the base argument parser, and add options on top.

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter, add_help=True)

    
    parser.add_argument("--crystal",
                        type=str,
                        required=True,
                        help="Put the Crystal Type")

    parser.add_argument("--beam",
                        type=str,
                        required=True,
                        help="Put the Particle of the beam")


    #tools.check_options_validity(parser)

    return parser

if __name__ == "__main__":

    with open('data.yaml', 'r') as file:
        runs_info = yaml.safe_load(file)
    
    print(runs_info)


    args = argparser().parse_args()

    wf = {}
    df = {}
    mean = {}
    fig_path = f'figures/{args.crystal}/{args.beam}'
    if not os.path.exists(fig_path):
        os.makedirs(fig_path)
        print(f"Cartella '{fig_path}' creata.")
    else:
        print(f"Cartella '{fig_path}' esiste già.")
    print('Crystal: ', args.crystal)
    print('Particle: ', args.beam)
    channels = [1, 2]
    for angle, run_number in runs_info[args.crystal][args.beam].items():
        print(f'Reading Run {run_number} - Angle {angle}')

        if os.path.exists(f'./parqs/info_run{run_number}.parq'):
            df[angle] = pd.read_parquet(f'./parqs/info_run{run_number}.parq')
        else:
            wf[angle], df[angle] = read_waveform(run_number)
            df[angle].to_parquet(f'./parqs/info_run{run_number}.parq')
        '''
        fig, ax = plt.subplots(len(channels), 1, sharex = True, figsize = (21, 9*len(channels)))
        for i, channel in enumerate(channels):
            event_id = 123
            ax[i].plot(wf[angle][0]['times'], wf[angle][event_id][channel], label='Dati', color='darkcyan', linestyle='-')
            ax[i].plot(wf[angle][0]['times'], wf[angle][event_id][f'{channel}media'], label='Dati', color='darkorange', linestyle='-')
            ax[i].set_ylabel('Amplitude [mV]', fontsize = 20)
            ax[i].set_title(f'Channel {channel}', fontsize = 30)
            ax[i].grid(alpha = 1, which = 'both')
        plt.xlabel('time [ns]', fontsize = 20)
        plt.tight_layout()
        plt.savefig(f'{fig_path}/waveforms_event{event_id}_run{run_number}_angle{angle}.png' ,dpi = 300)
'''
        data = df[angle][f'amplitude_media_channel2']
        M = np.percentile(data, 90)
        m = np.percentile(data, 0)
        
        
        if os.path.exists(f'./npys/Mean_Waveform_run{run_number}_channel2.npy'):
            mean[angle] = np.load(f'./npys/Mean_Waveform_run{run_number}_channel2.npy')
        else:
            mean[angle] = sum_waveform(wf[angle], 2, maximum = True, ampl_max = M, ampl_min = m)
            np.save(f'./npys/Mean_Waveform_run{run_number}_channel2.npy', mean[angle])
            del wf[angle]
            
        for channel in channels:
            plt.figure()
            hist, bin_edges, __ = plt.hist(df[angle][f'amplitude_media_channel{channel}'], bins = 200, histtype = 'step', linewidth = 3, color = 'darkcyan')
            if channel == 2:
                plt.axvline(m, linestyle='--', label=f'0° percentile')
                plt.axvline(M, linestyle='--', label=f'90° percentile')
            plt.grid(alpha = 1, which = 'both')
            
            plt.title(f'Amplitude Spectrum Run {run_number} - Channel {channel}')
            plt.xlabel('Amplitude [mV]')
            plt.ylabel('Event Count')
            plt.tight_layout()
            plt.savefig(f'{fig_path}/AmplitudeSpectrum_run{run_number}_angle{angle}_channel{channel}.png', dpi = 300)
            print(colored(f"Amplitude of {run_number} - Channel {channel} Saved","green"))

    maxs = {}
    mins = {}
    frac = {}
    frac2 = {}
    diffs = {}
    satu = {}
    plt.figure()
    for angle, run_number in runs_info[args.crystal][args.beam].items():
       
        diff = mean[angle]
        plt.plot(diff, label = f'{angle}')
        
        index = np.argmax(diff[300:340])
        index = index + 300
        
        index_min = np.argmin(diff[index:340])
        index_min = index_min + index
        
        index_satu = np.argmax(diff[index_min:500])
        index_satu = index_satu + index_min
        
        maxs[angle] = diff[index]
        mins[angle] = diff[index_min]
        satu[angle] = diff[index_satu]
        frac[angle] = maxs[angle]/mins[angle]
        frac2[angle] = maxs[angle]/satu[angle]
        diffs[angle] = maxs[angle] - mins[angle]
        plt.plot(index, maxs[angle], 'x', color='darkred', markersize = 15)
        plt.plot(index_min, mins[angle], 'x', color='darkcyan', markersize = 15)
        plt.plot(index_satu, satu[angle], 'x', color='darkblue', markersize = 15)
        
        plt.legend(ncol = 2, fontsize = 20)
        plt.xlim(100,500)
        plt.xlabel('')
    plt.tight_layout()
    plt.title(f'{args.crystal} - {args.beam}')
    plt.savefig(f'{fig_path}/Mean_waveforms_AngleScan_{args.crystal}_{args.beam}.png', dpi = 300)
    
    plt.figure()
    plt.plot(frac.keys(), frac.values(), 'o-', color = 'darkcyan', markersize = 15)
    plt.xlabel('Angle')
    plt.ylabel('Max/Min in the Cherenkov range')
    plt.title(f'Cherenkov Yield - {args.crystal} - {args.beam}')
    plt.grid(alpha = 1)
    plt.tight_layout()
    plt.savefig(f'{fig_path}/Cherenkov_Yield_Angle_{args.crystal}_{args.beam}_MaxMin.png', dpi = 300)

    plt.figure()
    plt.plot(frac2.keys(), frac2.values(), 'o-', color = 'darkcyan', markersize = 15)
    plt.xlabel('Angle')
    plt.ylabel('Picco Prompt/Picco Delayed')
    plt.title(f'Cherenkov Yield - {args.crystal} - {args.beam}')
    plt.grid(alpha = 1)
    plt.tight_layout()
    plt.savefig(f'{fig_path}/Cherenkov_Yield_Angle_{args.crystal}_{args.beam}_PromptDelayed.png', dpi = 300)
        
    