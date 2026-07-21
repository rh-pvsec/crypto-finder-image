import pytest
from pytest_container import Container, BindMount

def pytest_addoption(parser):
    parser.addoption(
        "--image",
        action="store",
        default="images.paas.redhat.com/exd-sp-guild-security/rh-crypto-scanner-image:latest",
        help="Specify target image"
    )

def pytest_generate_tests(metafunc):
    # Check if the test is using the standard pytest-container fixture
    if "container" in metafunc.fixturenames:
        image = metafunc.config.getoption("--image")
        
        # Create the pytest-container specific object
        dynamic_container = Container(
            url=image,
            volume_mounts=[
                BindMount(container_path="/workspace", host_path="./sample"),
            ],
    custom_entry_point="/bin/bash",
        )
        
        # Parametrize indirectly so pytest-container handles the lifecycle!
        metafunc.parametrize("container", [dynamic_container], indirect=True)
