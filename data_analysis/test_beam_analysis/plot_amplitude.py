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

    parser.add_argument("--max_events",
                        action = 'store_true',
                        help="If you want to see only the first `number_event` events")
    parser.add_argument("--number_events",
                        type = int,
                        help="Set the number of events (makes sense only with the option --max_events)")
    
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
    not_all_events = args.max_events
    number_events = args.number_events

    print(f'List of runs: {run_numbers}')
    wf = {}
    df = {}
    for run_number in run_numbers:
        wf[run_number], df[run_number] = read_waveform(run_number, save_cand, not_all_events, number_events)

    for channel in channels:
        plt.subplot()
        hist, bin_edges, __ = plt.hist(df[run_number][f'amplitude_media_channel{channel}'], bins = 200, range = (0, 100), histtype = 'step', linewidth = 3, color = 'darkcyan')
        plt.grid(alpha = 1, which = 'both')
        plt.yscale('log')
        plt.title(f'Amplitude Spectrum run {run_number} - Channel {channel}')
        plt.xlabel('Amplitude [mV]')
        plt.ylabel('Event Count')
        plt.tight_layout()
        plt.savefig(f'figures/AmplitudeSpectrum_run{run_number}_channel{channel}.png', dpi = 300)
        print(colored(f"Amplitude of {run_number} - Channel {channel} Saved","green"))