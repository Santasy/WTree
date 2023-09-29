import numpy as np
import pandas as pd
import matplotlib as mpl

# [1] Vectors

kvec = np.array([2**i for i in range(3, 16)])  # [8 - 32768]
single_versions_bytesvec = np.array(
    [2**i for i in range(6, 11)])  # [64 - 1024]

v_type_names = np.array(["ushort", "uint", "ulong"])
v_gen_names = np.array(["uniform", "normal", "bimodal"])
v_rivals = np.array(["BST", "RB", "BTREE"])
v_struct_names = np.concatenate([np.array(["WTREE"]), v_rivals])


class Rutines_metadata:
    prefix = ""
    steps = 0
    step_names = []
    short_stepnames = []
    arr_minmax = []

    def __init__(self, prefix, step_names, short_stepnames, arr_minmax):
        self.prefix = prefix
        self.steps = len(step_names)
        self.step_names = step_names
        self.short_stepnames = short_stepnames
        self.arr_minmax = arr_minmax


metadata_full = Rutines_metadata("full",
                                 ["Insert from 0 to 0.75N", "Insert from 0.75N to N", "Search Existent 0.25N Keys",
                                  "Search Inexistent 0.25N Keys", "Remove from 0.25N", "Random Search 0.25N Keys"],
                                 ["Insert from empty", "Populated Insert", "Search existent keys",
                                  "Search non-existent keys", "Remove", "Random Search"],
                                 [1] * 6
                                 )

metadata_mma = Rutines_metadata("mma",
                                ["Insert N elements", "Interpersed 0.3N Pops",
                                    "Insert 0.3N elements"],
                                ["Insert N", "0.3N Pops", "Insert 0.3N"],
                                [1, 0, 1]
                                )

metadata_mmc = Rutines_metadata("mmc",
                                ["Insert N elements", "0.15N Popmins",
                                    "0.15N Popmaxs", "Insert 0.3N elements"],
                                ["Insert N", "0.15N Popmins",
                                    "0.15N Popmaxs", "Insert 0.3N"],
                                [1, 0, 0, 1]
                                )

pcolor = mpl.colormaps['tab10']


# [2] Dictionaries

edic_types = {
    "s": [2],
    "i": [4],
    "l": [8],
    "a": [2, 4, 8],
}

edic_generators = {
    "u": [0],
    "n": [1],
    "b": [2],
    "a": [0, 1, 2],
}

edic_sizes = {
    "s":  int(100_000),
    "n":  int(1_000_000),
    "m":  int(5_000_000),
    "b":  int(10_000_000),
    "l":  int(50_000_000),
    "xs": int(100_000_000),
    "xl": int(2_147_483_647),
}

edic_kvec = {
    "n":  kvec,  # normal
    "s":  kvec[4:-3],  # short
    "t":  np.array([2**i for i in range(16, 21)]),  # big stable
    "u":  np.array([2**i for i in range(16, 31)]),  # all uint
}

# === Cache sizes for my machine:
# L1d cache size:   32K
#  L2 cache size:  512K
#  L3 cache size: 8192K

single_versions = {
    "010": {"TARGETBYTES": single_versions_bytesvec},
    "011": {"TARGETBYTES": single_versions_bytesvec},
    "020": {"TARGET_NODE_BYTES": [128, 192, 256, 320],
            "TARGET_BLOQ_BYTES": [1024*i for i in range(1, 32, 4)]},
}
