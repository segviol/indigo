import os
import subprocess
import argparse
import re
import sys
from tqdm import tqdm
import logging
import colorlog
import json

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
    test_count = 0
    success_count = 0
    fail_list = []

    files = os.listdir(dir)
    for file in tqdm(files):
        new_path = os.path.join(dir, file)
        if os.path.isdir(new_path) and args.recursively:
            test_dir(new_path)
        elif file.split('.')[-1] == 'sy':
            try:
                test_count += 1
                prefix = file.split('.')[0]
                f = open("info.txt", 'w')

                subprocess.run(
                    [args.compiler_path, new_path, "-d", "-o", "tmp.s"],
                    stdout=f)

                subprocess.run([
                    'gcc', 'tmp.s', args.linked_library_path, '-march=armv7-a',
                    "-o", "tmp"
                ],
                               stdout=f)

                f_out = open("./output.txt", 'w')
                if os.path.exists(os.path.join(dir, f"{prefix}.in")):
                    f = open(os.path.join(dir, f"{prefix}.in"), 'r')
                    subprocess.call(["./tmp"],
                                    stdin=f,
                                    stdout=f_out,
                                    timeout=8)
                else:
                    subprocess.call(["./tmp"], stdout=f_out, timeout=8)
                f_out.close()
                with open("./output.txt", 'r') as f:
                    my_output = f.readlines()
                with open(os.path.join(dir, f"{prefix}.out"), 'r') as f:
                    std_output = f.readlines()
                std_output[-1] = std_output[-1].strip()
                if my_output != std_output:
                    logger.error(f"mismatched output for {new_path}: ")
                    logger.error(
                        f"\texpected: {std_output}\n\tgot {my_output}")
                    fail_list.append({
                        "file": file,
                        "reason": "WA",
                        "expected": std_output,
                        "got": my_output
                    })
                else:
                    logger.info(f"Successfully passed {prefix}")
                    success_count += 1
                f.close()
            except subprocess.TimeoutExpired:
                logger.error(f"{prefix} time out!(longer than 8 seconds)")
                fail_list.append({"file": file, "reason": "timeout"})
    logger.info(f"{success_count} of {test_count} tests passed")
    fail_list_json = json.dumps(fail_list, indent=2, sort_keys=True)
    logger.info(f"Failed: {fail_list_json}")
    return 0 if success_count == test_count else 1


result = test_dir(root_path)
exit(result)
