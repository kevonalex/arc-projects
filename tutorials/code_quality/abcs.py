# Abstract Base Classes
# Abstract Base Classes are frequently used to create interfaces that can be used when one process or function needs to satisfy multiple possible sub-processes/features

from abc import ABC

class PipelineStep(ABC):
    pass

