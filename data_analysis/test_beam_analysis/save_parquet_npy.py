from __init__ import *

__author__ = "Giovanni Gaudino"
__email__ = "gaudino@na.infn.it"

from termcolor import colored
import numpy as np
import argparse

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
        df.to_parquet(f'/eos/home-g/ggaudino/tb_analysis/parqs/info_run{run_number}.parq')
        print('I saved the parquet file')
        for channel in channels:
            wf_media = sum_waveform(wf, channel)
            np.save(f'/eos/home-g/ggaudino/tb_analysis/npy/Mean_Waveform_run{run_number}_channel{channel}.npy', wf_media)
            print('I saved the numpy file')
        del wf
        del df
        del wf_media
        