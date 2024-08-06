from __init__ import *

__author__ = "Giovanni Gaudino"
__email__ = "gaudino@na.infn.it"

from termcolor import colored
import numpy as np
import argparse
import matplotlib.pyplot as plt
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

def parse_run_sequence(run_sequence):
    """
    Parse the run sequence argument.
    
    Accepts either a list of numbers or a range in the form of 'start:end'.
    """
    runs = []
    for item in run_sequence:
        if ':' in item:
            start, end = map(int, item.split(':'))
            runs.extend(range(start, end + 1))
        else:
            runs.append(int(item))
    return runs


def argparser():
    """
    Parse options as command-line arguments.
    """

    # Re-use the base argument parser, and add options on top.

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawTextHelpFormatter, add_help=True)

    parser.add_argument("--runs",
                        nargs='+',
                        type=str,
                        required=True,
                        help="List of run numbers or ranges in the form start:end")
    
    parser.add_argument("--channels",
                        nargs='+',
                        type=str,
                        required=True,
                        help="List of channels or ranges in the form start:end")
    
    parser.add_argument("--save_candidate",
                        action = 'store_true',
                        help="If True, saves all possible peaks in the waveform.")

    #tools.check_options_validity(parser)

    return parser

if __name__ == "__main__":

    args = argparser().parse_args()
    run_numbers = parse_run_sequence(args.runs)
    channels = parse_run_sequence(args.channels)
    save_cand = args.save_candidate

    print(f'List of runs: {run_numbers}')
    for run_number in run_numbers:
        print(f'Reading Run {run_number}')
        wf, df = read_waveform(run_number, save_cand)
    
    
        fig, ax = plt.subplots(len(channels), 1, sharex = True, figsize = (21, 9*len(channels)))
        for i, channel in enumerate(channels):
            event_id = 2
            ax[i].plot(wf[0]['times'], wf[event_id][channel], label='Dati', color='darkcyan', linestyle='-')
            ax[i].plot(wf[0]['times'], wf[event_id][f'{channel}media'], label='Dati', color='darkorange', linestyle='-')
            ax[i].set_ylabel('Amplitude [mV]', fontsize = 20)
            ax[i].set_title(f'Channel {channel}', fontsize = 30)
            ax[i].grid(True)
        plt.xlabel('time [ns]', fontsize = 20)
        plt.tight_layout()
        plt.savefig(f'figures/waveforms_event{event_id}_run{run_number}.png' ,dpi = 300)
        