import json
from typing import List

from defusedxml import ElementTree as SafeElementTree  # type:ignore[import-untyped]

from cyclonedx.exception import MissingOptionalDependencyException
from cyclonedx.model import crypto
from cyclonedx.model.bom import Bom
from cyclonedx.model.component import Component, ComponentType
from cyclonedx.schema import OutputFormat, SchemaVersion
from cyclonedx.validation import make_schemabased_validator
from cyclonedx.validation.json import JsonValidator


def get_spec_version(data: str) -> str:
    """
    This function checks if data is a valid json, it's a bom
    with format CycloneDX and has a specVersion defined.
    """

    try:
        j = json.loads(data)
    except:
        return None
    if j.get("bomFormat") != "CycloneDX":
        return None
    return j.get("specVersion", None)

def check_cbom(data: str, algorithms_detected: List[str]):
    """
    This function checks if data is a valid CycloneDX format.
    Checks are against 1.6 schema using cyclonedx python library
    
    """

    spec_version = get_spec_version(data)
    if spec_version not in ["1.6", "1.7"]:
        return False

    my_json_validator = JsonValidator(SchemaVersion.from_version(spec_version))

    try:
        json_validation_errors = my_json_validator.validate_str(data)
        if json_validation_errors:
            print('JSON invalid', 'ValidationError:', repr(json_validation_errors), sep='\n')
            return False
    except MissingOptionalDependencyException as error:
        print('JSON-validation was skipped due to', error)

    bom = Bom.from_json( # type: ignore[attr-defined]
        data=json.loads(data))
    j=json.loads(data)
    print(json.dumps(j, indent=4))
    crypto_assets = [ c for c in bom.components if c.type == ComponentType.CRYPTOGRAPHIC_ASSET ]
    algorithms = [ c for c in crypto_assets if c.crypto_properties.asset_type == crypto.CryptoAssetType.ALGORITHM ]
    algorithms_names = [ c.name for c in algorithms ]
    for algorithm_detected in algorithms_detected:    
        assert algorithm_detected in algorithms_names
    return True