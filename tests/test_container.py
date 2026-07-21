import pytest
from pytest_container import inspect
from pytest_container.container import Container, BindMount, ContainerData

from . import cbom


@pytest.mark.parametrize("filename",
    [
        "/usr/local/bin/crypto-finder",
        "/root/.opengrep/cli/latest/opengrep",
        "/usr/local/rules/crypto-rules",
        "/usr/local/rules/rh-crypto-rules",
    ]
)
def test_file_exist(container: ContainerData, filename: str):
    assert container.connection.file(filename).exists

def test_openssl_algorithm(container: ContainerData):
    # Would be better if we can inspect the original ENTRYPOINT
    # from the image (without creating the container)
    args = [
        "crypto-finder",
        "scan", "--no-remote-rules",
        "--rules-dir", "/usr/local/rules/crypto-rules",
        "--rules-dir", "/usr/local/rules/rh-crypto-rules",
        "--format", "cyclonedx",
        "--timeout", "60m"
    ]
    args.append("/workspace/openssl_usage")
    out = container.connection.run(' '.join(args))
    assert out.succeeded

    # Test against CycloneDX Schema and confirm finding
    assert cbom.check_cbom(out.stdout, ["SHA-512"])
