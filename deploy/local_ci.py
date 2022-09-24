#!/usr/bin/env python3

import os
import yaml
import shlex
import pathlib
import argparse
import subprocess
from collections import namedtuple

CiData = namedtuple("CiData", ["entries", "variables", "default", "args"])


def list_jobs(data: CiData):
    for name in sorted(data.entries.keys()):
        print(name)


def write_script(file, steps, name):
    file.write(r"echo -e '\x1b[1m%s\x1b[m'" % name)
    file.write("\n")
    for step in steps:
        file.write(step + "\n")


def exec_cmd(cmd_args):
    print(" ".join(map(shlex.quote, cmd_args)))
    subprocess.call(cmd_args)


def run_job(data: CiData):
    job_data = data.default
    job_data.update(data.entries[data.args.job])

    image = job_data["image"]

    git_recursive = data.variables.get("GIT_SUBMODULE_STRATEGY") == "recursive"

    parent_dir = data.args.work_path
    job_filename = data.args.job.replace(":", "_").replace("/", "_")
    work_dir = parent_dir / job_filename

    if data.args.local_files:
        work_dir = data.args.clone_url
    elif not work_dir.exists():
        parent_dir.mkdir(parents=True, exist_ok=True)
        git_args = [data.args.clone_url, job_filename, "-b", data.args.branch]
        if git_recursive:
            git_args.append("--recursive")
        exec_cmd(["git", "-C", str(parent_dir), "clone"] + git_args)
    else:
        exec_cmd(["git", "-C", str(work_dir), "pull"])
        if git_recursive:
            exec_cmd(["git", "-C", str(work_dir), "submodule", "update"])

    script_filename = parent_dir / (job_filename + ".sh")
    with open(script_filename, "w") as script_file:
        scripts = ["before_script", "script", "after_script"]
        script_file.write("#!/usr/bin/env bash\nset -ex\ncd /ci\n")
        for script in scripts:
            write_script(script_file, job_data.get(script, []), script)
    script_filename.chmod(0o755)

    script_wrapper_filename = parent_dir / "ci_wrapper.sh"
    with open(script_wrapper_filename, "w") as script_file:
        script_file.write("#!/usr/bin/env bash\ncd /ci\n/ci.sh || bash")
    script_wrapper_filename.chmod(0o755)

    docker_vars = ["-e", "DISPLAY"]
    for item in data.variables.items():
        docker_vars.append("-e")
        docker_vars.append("%s=%s" % item)

    if data.args.privileged:
        docker_vars.append("--privileged")

    exec_cmd([
        "docker",
        "run", "-it",
        "-v", "%s:/ci" % work_dir,
        "-v", "%s:/ci.sh" % script_filename,
        "-v", "%s:/ci_wrapper.sh" % script_wrapper_filename,
        "-v" "/tmp/.X11-unix",
        ] + docker_vars + [
        image,
        "bash", "-c", "/ci_wrapper.sh"
    ])


def main():
    root = pathlib.Path(__file__).parent.parent
    file = root / ".gitlab-ci.yml"

    parser = argparse.ArgumentParser()
    parser.add_argument("--input-file", "-i", default=file, type=pathlib.Path)

    sub = parser.add_subparsers()
    sub.add_parser("list-jobs", aliases=["list", "ls"]).set_defaults(action=list_jobs)

    sub_run = sub.add_parser("run")
    sub_run.set_defaults(action=run_job)
    sub_run.add_argument("job")
    sub_run.add_argument("--local-files", action="store_true", help="Use the directory from --clone-url as is instead of cloning from git")
    sub_run.add_argument("--privileged", action="store_true")
    sub_run.add_argument("--clone-url", default=str(root)) # default="https://gitlab.com/mattbas/glaxnimate.git")
    sub_run.add_argument("--work-path", default=pathlib.Path("/tmp/glaxnimate_ci"), type=pathlib.Path)
    sub_run.add_argument("--branch", "-b", default="master")

    ns = parser.parse_args()

    with open(ns.input_file) as infile:
        doc = yaml.load(infile.read(), Loader=yaml.FullLoader)

    default = doc.pop("default", {})
    variables = doc.pop("variables", {})

    ns.action(CiData(doc, variables, default, ns))


if __name__ == "__main__":
    main()
