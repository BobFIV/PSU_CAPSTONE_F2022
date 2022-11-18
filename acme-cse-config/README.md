# ACME CSE Configuration
This directory contains code and instructions for configuring the [ACME CSE](https://github.com/ankraft/ACME-oneM2M-CSE).  

## Install Flex Container Definitions
FlexContainers are a unique resource type in oneM2M that allows for specialized definitions to be created.  
In order to make the ACME CSE aware of our specific FlexContainer specialization, you will need to copy in [this `.fcp` file](https://github.com/BobFIV/PSU_CAPSTONE_F2022/blob/main/acme-cse-config/traffic-light-intersection.fcp) into the `init/` directory of ACME.

See [this README](https://github.com/ankraft/ACME-oneM2M-CSE/blob/master/docs/Importing.md#flexcontainers) from the ACME CSE for more information about FlexContainer specializations. 

## Startup
Once your ACME CSE is configured with the Flex container definition, you can launch it with the following command:  
`python3.8 -m acme --http-port 8080`

After starting the ACME CSE, follow the below instructions for enabling access for the Dashboard Web App.

## ACP Modification
In order to allow the Dashboard Web App to create notifications for the creation of intersection AE's, the root ACP of the CSE has to be modified.  
Using ACME's REST Web UI, add the following Access Control Record to the `createACPs` policy (add it as an element in the `pv.acr` array):
```
{
    "acor": [
        "Cdashboard",
        "Cdashboard1",
        "Cdashboard2",
        "Cdashboard3"
    ],
    "acod": [
        {
            "chty": [
                23
            ]
        }
    ],
    "acop": 1
}
```

Making the above modification should leave your `createACPs` policy looking something like this:
```
{
    "m2m:acp": {
        "rn": "acpCreateACPs",
        "ri": "acpCreateACPs",
        "pi": "id-in",
        "pv": {
            "acr": [
                {
                    "acor": [
                        "all"
                    ],
                    "acod": [
                        {
                            "chty": [
                                1
                            ]
                        }
                    ],
                    "acop": 1
                },
                {
                    "acor": [
                        "Cdashboard",
                        "Cdashboard1",
                        "Cdashboard2",
                        "Cdashboard3"
                    ],
                    "acod": [
                        {
                            "chty": [
                                23
                            ]
                        }
                    ],
                    "acop": 1
                }
            ]
        },
        "pvs": {
            "acr": [
                {
                    "acor": [
                        "CAdmin"
                    ],
                    "acop": 63
                }
            ]
        },
        "ct": "20221105T011811,976968",
        "lt": "20221108T022105,269094",
        "et": "20231105T011811,977028",
        "ty": 1
    }
}
```  
This modification allows AE's with the originators of `Cdashboard`, and `Cdashboard1` through `Cdashboard4` to create Subscription resources in the CSE root.  
By adding all of these originators, multiple web dashboards can be opened simultaneously.  
If you need more than the available 5 originators, you can always add more to the `acor` array.
