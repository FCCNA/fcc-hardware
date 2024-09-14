import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
from termcolor import colored
import argparse
from __init__ import *

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

