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
    
    crystal = args.crystal
    beam = args.beam

    fig_path = f'figures/{crystal}/{beam}'
    if not os.path.exists(fig_path):
        os.makedirs(fig_path)
        print(f"Cartella '{fig_path}' creata.")
    else:
        print(f"Cartella '{fig_path}' esiste già.")
    print('Crystal: ', crystal)
    print('Particle: ', beam)
    channels = [1, 2]
    for angle, run_number in runs_info[crystal][beam].items():
        print(f'Reading Run {run_number} - Angle {angle} - Crystal {crystal}')

        if os.path.exists(f'./parqs/info_run_{crystal}_{angle}.parq'):
            df[angle] = pd.read_parquet(f'./parqs/info_run_{crystal}_{angle}.parq')
        else:
            wf[angle], df[angle] = mixing_run(run_number)
            df[angle].to_parquet(f'./parqs/info_run_{crystal}_{angle}.parq')

        data = df[angle][f'amplitude_media_channel2']
        M = np.percentile(data, 90)
        m = np.percentile(data, 0)
        
        
        if os.path.exists(f'./npys/Mean_Waveform_run_{crystal}_{angle}_channel2.npy'):
            mean[angle] = np.load(f'./npys/Mean_Waveform_run_{crystal}_{angle}_channel2.npy')
        else:
            mean[angle] = sum_waveform(wf[angle], 2, maximum = True, ampl_max = M, ampl_min = m)
            np.save(f'./npys/Mean_Waveform_run_{crystal}_{angle}_channel2.npy', mean[angle])
            del wf[angle]
            
        for channel in channels:
            plt.figure()
            hist, bin_edges, __ = plt.hist(df[angle][f'amplitude_media_channel{channel}'], bins = 200, histtype = 'step', linewidth = 3, color = 'darkcyan')
            if channel == 2:
                plt.axvline(m, linestyle='--', label=f'0° percentile')
                plt.axvline(M, linestyle='--', label=f'90° percentile')
            plt.grid(alpha = 1, which = 'both')
            
            plt.title(f'Amplitude Spectrum Crystal {crystal} - Angle {angle} - Channel {channel}')
            plt.xlabel('Amplitude [mV]')
            plt.ylabel('Event Count')
            plt.tight_layout()
            plt.savefig(f'{fig_path}/AmplitudeSpectrum_run{crystal}_{angle}_angle{angle}_channel{channel}.png', dpi = 300)
            print(colored(f"Amplitude of Crystal {crystal} - Angle {angle} - Channel {channel} Saved","green"))

    maxs = {}
    mins = {}
    frac = {}
    frac2 = {}
    diffs = {}
    satu = {}
    plt.figure()
    for angle in runs_info[crystal][beam].keys():
       
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
    plt.title(f'{crystal} - {beam}')
    plt.savefig(f'{fig_path}/Mean_waveforms_AngleScan_{crystal}_{beam}.png', dpi = 300)
    
    plt.figure()
    plt.plot(frac.keys(), frac.values(), 'o-', color = 'darkcyan', markersize = 15)
    plt.xlabel('Angle')
    plt.ylabel('Max/Min in the Cherenkov range')
    plt.title(f'Cherenkov Yield - {crystal} - {beam}')
    plt.grid(alpha = 1)
    plt.tight_layout()
    plt.savefig(f'{fig_path}/Cherenkov_Yield_Angle_{crystal}_{beam}_MaxMin.png', dpi = 300)

    plt.figure()
    plt.plot(frac2.keys(), frac2.values(), 'o-', color = 'darkcyan', markersize = 15)
    plt.xlabel('Angle')
    plt.ylabel('Picco Prompt/Picco Delayed')
    plt.title(f'Cherenkov Yield - {crystal} - {beam}')
    plt.grid(alpha = 1)
    plt.tight_layout()
    plt.savefig(f'{fig_path}/Cherenkov_Yield_Angle_{crystal}_{beam}_PromptDelayed.png', dpi = 300)
        
    