import os
import subprocess
import argparse
import re
import sys
from tqdm import tqdm
import logging
import colorlog
import json
import signal

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
parser.add_argument('test_path',
                    metavar='test_path',
                    type=str,
                    help='the path of the test codes')
parser.add_argument('-r',
                    "--recursively",
                    help="recursively for sub dirs",
                    action="store_true")
args = parser.parse_args()
root_path = args.test_path


def test_dir(dir):
    num_tested = 0
    num_passed = 0
    fail_list = []
    pass_list = []

    files = os.listdir(dir)
    for file in tqdm(files):
        new_path = os.path.join(dir, file)
        if os.path.isdir(new_path) and args.recursively:
            test_dir(new_path)
        elif file.split('.')[-1] == 'sy':
            num_tested += 1
            prefix = file.split('.')[0]
            try:
                f = open("info.txt", 'w')

                subprocess.run(
                    [args.compiler_path, new_path, "-d", "-o", "tmp.s"],
                    stdout=f)

                subprocess.run([
                    'gcc', 'tmp.s', args.linked_library_path, '-march=armv7-a',
                    "-o", "tmp"
                ],
                               stdout=f)

                if os.path.exists(os.path.join(dir, f"{prefix}.in")):
                    f = open(os.path.join(dir, f"{prefix}.in"), 'r')
                    process = subprocess.run(["./tmp"],
                                             stdin=f,
                                             capture_output=True,
                                             timeout=8)
                else:
                    process = subprocess.run(["./tmp"],
                                             capture_output=True,
                                             timeout=8)

                my_output = str(process.stdout).splitlines()
                return_code = process.returncode

                if return_code < 0:
                    sig = -return_code
                    sig_def = signal.strsignal(sig)
                    logger.error(
                        f"{new_path} raised a runtime error with signal {sig} ({sig_def})"
                    )
                    fail_list.append({
                        "file": file,
                        "reason": "runtime_error",
                        "got": my_output,
                        "signal": sig_def
                    })

                with open(os.path.join(dir, f"{prefix}.out"), 'r') as f:
                    std_output = f.readlines()

                std_output[-1] = std_output[-1].strip()

                if my_output != std_output:
                    logger.error(
                        f"mismatched output for {new_path}: \nexpected: {std_output}\ngot {my_output}"
                    )
                    fail_list.append({
                        "file": file,
                        "reason": "wrong_output",
                        "expected": std_output,
                        "got": my_output
                    })
                else:
                    logger.info(f"Successfully passed {prefix}")
                    pass_list.append(file)
                    num_passed += 1
                f.close()

            except subprocess.TimeoutExpired as t:
                logger.error(
                    f"{prefix} time out!(longer than {t.timeout} seconds)")
                fail_list.append({
                    "file": file,
                    "reason": "timeout",
                    "got": t.stdout
                })

    logger.info(f"passed {num_passed} of {num_tested} tests")

    if fail_list.count() > 0:
        fail_list_json = json.dumps(fail_list, indent=2, sort_keys=True)
        logger.error(f"Failed tests: {fail_list_json}")

    return {
        "num_tested": num_tested,
        "num_passed": num_passed,
        "num_failed": num_tested - num_passed,
        "passed": pass_list,
        "failed": fail_list
    }


result = test_dir(root_path)
logger.log(result)
if result.num_failed != 0:
    exit(1)
else:
    exit(0)
