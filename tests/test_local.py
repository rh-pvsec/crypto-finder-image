import os
import subprocess
import pytest

from . import cbom

RULES = [
        #"/usr/local/rules/crypto-rules",
        "/usr/local/rules/open-crypto-rules",
        "/usr/local/rules/rh-crypto-rules",
        ]

@pytest.mark.parametrize("filename",
    [
        "/usr/local/bin/crypto-finder",
        "/root/.opengrep/cli/latest/opengrep",
    ]
)
def test_file_exist(filename: str):
    assert os.path.isfile(filename)

@pytest.mark.parametrize("dirname", RULES)
def test_dir_exist(dirname: str):
    assert os.path.isdir(dirname)

def test_openssl_algorithm():
    # Would be better if we can inspect the original ENTRYPOINT
    # from the image (without creating the container)

    args = [
        "crypto-finder",
        "scan", "--no-remote-rules"
        ] + [
            x for f in RULES for x in ("--rules-dir", f)
        ] + [
        "--format", "cyclonedx",
        "--timeout", "60m"
    ]
    args.append("sample/openssl_usage")
    output = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert output.check_returncode

    # Test against CycloneDX Schema and confirm rh-crypto-rule finding
    assert cbom.check_cbom(output.stdout, ["other"])

    # Test against CycloneDX Schema and confirm crypto_rules finding
    # assert cbom.check_cbom(output.stdout, ["SHA-512"])
