import os
from os import link, path
from re import fullmatch
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

output_folder_name = "output"


def format_compiler_output_file_name(name: str, ty: str):
    name = name.replace('/', '_')
    return f"{output_folder_name}/compiler-{ty}-{name}.txt"


def format_linker_output_file_name(name: str, ty: str):
    name = name.replace('/', '_')
    return f"{output_folder_name}/linker-{ty}-{name}.txt"


def test_dir(dir):
    num_tested = 0
    num_passed = 0
    fail_list = []
    pass_list = []

    files = os.listdir(dir)
    for file in tqdm(files):
        new_path = os.path.join(dir, file)
        if os.path.isdir(new_path) and args.recursively:
            result = test_dir(new_path)
            num_tested += result["num_tested"]
            num_passed += result["num_passed"]
            fail_list.extend(result["failed"])
            pass_list.extend(result["passed"])
        elif file.split('.')[-1] == 'sy':
            num_tested += 1
            prefix = file.split('.')[0]

            try:
                compiler_output = subprocess.run(
                    [args.compiler_path, new_path, "-d", "-o", "tmp.s"],
                    capture_output=True,
                    timeout=15)

                # compiler error
                if compiler_output.returncode != 0:
                    logger.error(f"{new_path} encountered a compiler error")

                    with open(
                            format_compiler_output_file_name(
                                new_path, 'stdout'), "w") as f:
                        f.write(compiler_output.stdout.decode('utf-8'))

                    fail_list.append({
                        "file": new_path,
                        "reason": "compile_error",
                        "return_code": compiler_output.returncode
                    })
                    continue

                link_output = subprocess.run([
                    'gcc', 'tmp.s', args.linked_library_path, '-march=armv7-a',
                    "-o", "tmp"
                ],
                                             capture_output=True)

                # compiler error
                if link_output.returncode != 0:
                    logger.error(f"{new_path} encountered a linker error")

                    with open(
                            format_linker_output_file_name(new_path, 'stdout'),
                            "w") as f:
                        f.write(link_output.stdout.decode('utf-8'))
                    with open(
                            format_compiler_output_file_name(
                                new_path, 'stdout'), "w") as f:
                        f.write(compiler_output.stdout.decode('utf-8'))
                    with open(
                            format_linker_output_file_name(new_path, 'stderr'),
                            "w") as f:
                        f.write(link_output.stderr.decode('utf-8'))

                    fail_list.append({
                        "file": file,
                        "reason": "link_error",
                        "return_code": link_output.returncode
                    })
                    continue

            except subprocess.TimeoutExpired as t:
                logger.error(
                    f"{prefix} compiler time out!(longer than {t.timeout} seconds)"
                )
                stdout = t.stdout
                if stdout != None:
                    str_stdout = stdout.decode("utf-8")
                else:
                    str_stdout = ""

                with open(format_compiler_output_file_name(new_path, 'stdout'),
                          "w") as f:
                    f.write(stdout)

                fail_list.append({
                    "file": new_path,
                    "reason": "compiler_timeout",
                })

                continue

            try:
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

                return_code = process.returncode
                my_output = process.stdout.decode("utf-8").replace("\r", "")
                my_output = (my_output + "\n" + str(return_code)).strip()
                limited_output = my_output[0:2048]
                if len(my_output) > 2048: limited_output += "...(stripped)"

                if return_code < 0:
                    sig = -return_code
                    sig_def = signal.strsignal(sig)
                    logger.error(
                        f"{new_path} raised a runtime error with signal {sig} ({sig_def})"
                    )

                    with open(
                            format_compiler_output_file_name(
                                new_path, 'stdout'), "w") as f:
                        f.write(compiler_output.stdout.decode('utf-8'))

                    fail_list.append({
                        "file": new_path,
                        "reason": "runtime_error",
                        "got": limited_output,
                        "error": sig_def
                    })

                else:
                    with open(os.path.join(dir, f"{prefix}.out"), 'r') as f:
                        std_output = f.read().replace("\r", "").strip()

                    my_output_lines = [
                        x.strip() for x in my_output.splitlines()
                        if x.strip() != ''
                    ]
                    std_output_lines = [
                        x.strip() for x in std_output.splitlines()
                        if x.strip() != ''
                    ]

                    if my_output_lines != std_output_lines:
                        logger.error(
                            f"mismatched output for {new_path}: \nexpected: {std_output}\ngot {my_output}"
                        )
                        with open(
                                format_compiler_output_file_name(
                                    new_path, 'stdout'), "w") as f:
                            f.write(compiler_output.stdout.decode('utf-8'))

                        fail_list.append({
                            "file": file,
                            "reason": "wrong_output",
                            "expected": std_output,
                            "got": limited_output
                        })
                    else:
                        logger.info(f"Successfully passed {prefix}")
                        pass_list.append(new_path)
                        num_passed += 1
                    f.close()

            except subprocess.TimeoutExpired as t:
                logger.error(
                    f"{prefix} time out!(longer than {t.timeout} seconds)")
                stdout = t.stdout
                if stdout != None:
                    str_stdout = stdout.decode("utf-8")
                    if len(str_stdout) > 2048:
                        str_stdout = str_stdout[0:2048] + "..."
                else:
                    str_stdout = ""

                fail_list.append({
                    "file": new_path,
                    "reason": "timeout",
                    "got": str_stdout
                })

    logger.info(f"passed {num_passed} of {num_tested} tests")

    if len(fail_list) > 0:
        logger.error(f"Failed tests: {fail_list}")

    return {
        "num_tested": num_tested,
        "num_passed": num_passed,
        "num_failed": num_tested - num_passed,
        "passed": pass_list,
        "failed": fail_list
    }


result = test_dir(root_path)
logger.info(result)
if result["num_failed"] != 0:
    exit(1)
else:
    exit(0)
