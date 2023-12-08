import argparse
from itertools import product
from pathlib import Path

from jinja2 import Template


template_file = Path(__file__).expanduser().resolve().parent / "run_template.sh"


def build_run_dir(dump_dir: Path,  prefix: Path, data_size: int, num_iters: int, num_clients: int, use_lib_path_script: bool):
    client_exe = prefix / "bin" / "dyad_ucx_perftest_client"
    server_exe = prefix / "bin" / "dyad_ucx_perftest_server"
    lib_path_script = Path(__file__).expanduser().resolve().parent / "lib_path.sh"
    dir_name = "run_{}bytes_{}iters_{}clients".format(data_size, num_iters, num_clients)
    run_dir = dump_dir / dir_name
    if not run_dir.is_dir():
        run_dir.mkdir(parents=True)
    template_config = {
        "source_lib_path_script": "source {}".format(str(lib_path_script)) if use_lib_path_script else "",
        "server_exe": str(server_exe),
        "data_size": data_size,
        "num_clients": num_clients,
        "client_exe": str(client_exe),
        "num_iters": num_iters
    }
    template = None
    with open(template_file, "r") as f:
        template = Template(f.read())
    with open((run_dir / "run.sh"), "w") as f:
        f.write(template.render(template_config))


def main():
    parser = argparse.ArgumentParser("Create submissions scripts for this micro-benchmark")
    parser.add_argument("install_prefix", type=Path,
                        help="Install prefix for the micro-benchmark")
    parser.add_argument("--data_size", "-s", type=int, metavar="S", nargs="+",
                        help="Data size(s) to be transfered between client and server, in bytes")
    parser.add_argument("--num_iters", "-i", type=int, metavar="I", nargs="+",
                        help="Number(s) of iterations that each client should run for")
    parser.add_argument("--num_clients", "-c", type=int, metavar="C", nargs="+",
                        help="Number(s) of clients to run")
    parser.add_argument("--dump_dir", "-d", type=Path, default=(Path.cwd() / "dump_dir"),
                        help="Directory to dump configured runs")
    parser.add_argument("--use_lib_path_script", "-u", action="store_true",
                        help="If provided, add 'source lib_path.sh' to the submission script")
    args = parser.parse_args()
    prefix = args.install_prefix.expanduser().resolve()
    dump_dir = args.dump_dir.expanduser().resolve()
    if not dump_dir.is_dir():
        dump_dir.mkdir(parents=True)
    for cart_prod in product(args.data_size, args.num_iters, args.num_clients):
        dsize = cart_prod[0]
        niters = cart_prod[1]
        nclients = cart_prod[2]
        build_run_dir(dump_dir, prefix, dsize, niters, nclients, args.use_lib_path_script)

        
if __name__ == "__main__":
    main()