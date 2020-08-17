import os
import subprocess
import argparse
import re
from typing import Dict, List
from tqdm import tqdm
import logging
import colorlog
import signal
import csv

log_colors_config = {
    'DEBUG': 'white',  # cyan white
    'INFO': 'green',
    'WARNING': 'yellow',
    'ERROR': 'red',
    'CRITICAL': 'bold_red',
}

logger = logging.getLogger('logger_name')

console_handler = logging.StreamHandler()

file_handler = logging.FileHandler(filename='test.log',
                                   mode='a',
                                   encoding='utf8')

logger.setLevel(logging.DEBUG)
console_handler.setLevel(logging.DEBUG)
file_handler.setLevel(logging.INFO)

file_formatter = logging.Formatter(
    fmt=
    '[%(asctime)s.%(msecs)03d] %(filename)s -> %(funcName)s line:%(lineno)d [%(levelname)s] : %(message)s',
    datefmt='%H:%M:%S')
console_formatter = colorlog.ColoredFormatter(
    fmt='%(log_color)s[%(asctime)s]  : %(message)s',
    datefmt='%H:%M:%S',
    log_colors=log_colors_config)
console_handler.setFormatter(console_formatter)
file_handler.setFormatter(file_formatter)

if not logger.handlers:
    logger.addHandler(console_handler)
    logger.addHandler(file_handler)

console_handler.close()
file_handler.close()
parser = argparse.ArgumentParser(description='Automaticly test')
parser.add_argument("compiler_path",
                    metavar='compiler_path',
                    type=str,
                    help='the path of the compiler')
parser.add_argument("linked_library_path",
                    metavar='linked_library_path',
                    type=str,
                    help='the path of the linked library')
parser.add_argument("source_library_path",
                    metavar='source_library_path',
                    type=str,
                    help='the path of the library source')
parser.add_argument('test_path',
                    metavar='test_path',
                    type=str,
                    help='the path of the test codes')
parser.add_argument('-r',
                    "--recursively",
                    help="recursively for sub dirs",
                    action="store_true")
parser.add_argument('-t',
                    "--timeout",
                    default=120,
                    help="Kill tests after specified time",
                    type=float)

args = parser.parse_args()
root_path = args.test_path

output_folder_name = "output"


def decode_stderr_timer(stderr: str) -> float:
    time_match = re.search('TOTAL:\s*(\d+)H-(\d+)M-(\d+)S-(-?\d+)us\s*$',
                           stderr)
    if time_match == None:
        return None
    else:
        hrs = float(time_match.group(1))
        mins = float(time_match.group(2))
        secs = float(time_match.group(3))
        us = float(time_match.group(4))
        return hrs * 3600 + mins * 60 + secs + us / 1000000


stages = {
    "mine": [[args.compiler_path, "-o", "out.s", "$input"],
             ["gcc", "out.s", "$link_lib", "-march=armv7-a", "-o", "tmp"]],
    "gcc_o1": [[
        "gcc", "-xc", "$input", "$c_lib", "-march=armv7-a", "-std=c11", "-o",
        "tmp", "-O1"
    ]],
    "gcc_o2": [[
        "gcc", "-xc", "$input", "$c_lib", "-march=armv7-a", "-std=c11", "-o",
        "tmp", "-O2"
    ]],
    "gcc_ofast": [[
        "gcc", "-xc", "$input", "$c_lib", "-march=armv7-a", "-std=c11", "-o",
        "tmp", "-Ofast"
    ]],
    "clang_o2": [[
        "clang", "-xc", "$input", "$c_lib", "-march=armv7-a", "-std=c11", "-o",
        "tmp", "-O2"
    ]]
}

options = {
    "$link_lib": args.linked_library_path,
    "$c_lib": args.source_library_path
}


def test_dir(
    dir: str,
    runner: dict,
    options: dict,
) -> List[dict]:
    result = []
    files = os.listdir(dir)
    for file in tqdm(files):
        new_path = os.path.join(dir, file)
        if os.path.isdir(new_path) and args.recursively:
            child = test_dir(new_path, runner, options)
            result.combine(child)
        elif file.split('.')[-1] == 'sy':
            prefix = file.split('.')[0]
            options["$input"] = new_path
            job_result = {"file": file}
            for job in runner:
                logger.info(f"Running {file} with {job}")
                try:
                    for stage in runner[job]:
                        my_stage = [
                            (options[x] if options.get(x) != None else x)
                            for x in stage
                        ]
                        compiler_output = subprocess.run(my_stage,
                                                         capture_output=True,
                                                         timeout=60)

                        # compiler error
                        if compiler_output.returncode != 0:
                            logger.error(
                                f"{new_path} encountered a compiler error:\n{compiler_output.stderr.decode('utf8')}"
                            )
                            continue

                except subprocess.TimeoutExpired as t:
                    logger.error(
                        f"{prefix} compiler time out!(longer than {t.timeout} seconds)"
                    )
                    continue

                try:
                    if os.path.exists(os.path.join(dir, f"{prefix}.in")):
                        f = open(os.path.join(dir, f"{prefix}.in"), 'r')
                        process = subprocess.run(["./tmp"],
                                                 stdin=f,
                                                 capture_output=True,
                                                 timeout=args.timeout)
                    else:
                        process = subprocess.run(["./tmp"],
                                                 capture_output=True,
                                                 timeout=args.timeout)

                    return_code = process.returncode % 256

                    if return_code < 0:
                        sig = -return_code
                        sig_def = signal.strsignal(sig)
                        logger.error(
                            f"{new_path} raised a runtime error with signal {sig} ({sig_def})"
                        )

                    else:
                        stderr = process.stderr.decode("utf-8")
                        t = decode_stderr_timer(stderr)
                        if t != None:
                            job_result[job] = t
                            logger.info(f"job taken {t}s")
                        else:
                            logger.error(f"Cannot find timer in {stderr}")
                except:
                    logger.error(f"Error.")
                    pass

            result.append(job_result)


result = test_dir(root_path, stages, options)
print(result)
with open("result.csv", "w") as file:
    writer = csv.DictWriter(file, ["file"].extend(list(stages)))
    writer.writeheader
    for row in result:
        writer.writerow(row)
