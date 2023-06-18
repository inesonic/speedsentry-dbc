/*-*-c++-*-*************************************************************************************************************
* Copyright 2021 - 2023 Inesonic, LLC.
*
* GNU Public License, Version 3:
*   This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public
*   License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later
*   version.
*   
*   This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied
*   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
*   details.
*   
*   You should have received a copy of the GNU General Public License along with this program.  If not, see
*   <https://www.gnu.org/licenses/>.
********************************************************************************************************************//**
* \file
*
* This header implements the \ref ActiveResources class.
***********************************************************************************************************************/

#include <cstring>
#include <cstdint>

#include "region.h"
#include "active_resources.h"

ActiveResources::ActiveResources() {
    currentCustomerId = 0;
    std::memset(activeFlags, 0, sizeof(activeFlags));
}


ActiveResources::ActiveResources(
        CustomerId customerId
    ):currentCustomerId(
        customerId
    ) {
    std::memset(activeFlags, 0, sizeof(activeFlags));
}

ActiveResources::ActiveResources(const ActiveResources& other) {
    currentCustomerId = other.currentCustomerId;
    std::memcpy(activeFlags, other.activeFlags, sizeof(activeFlags));
}


ActiveResources::~ActiveResources() {}


bool ActiveResources::isValid() const {
    bool result;

    if (currentCustomerId != 0) {
        unsigned i=0;
        while (i < numberFlagElements && activeFlags[i] == 0) {
            ++i;
        }

        result = (i < numberFlagElements);
    } else {
        result = false;
    }

    return result;
}


void ActiveResources::setActive(ActiveResources::ValueType valueType, bool nowActive) {
    unsigned flagIndex  = valueType / entriesPerFlag;
    unsigned flagOffset = valueType % entriesPerFlag;
    Flags    mask       = 1ULL << flagOffset;

    if (nowActive) {
        activeFlags[flagIndex] |= mask;
    } else {
        activeFlags[flagIndex] &= ~mask;
    }
}


bool ActiveResources::isActive(ActiveResources::ValueType valueType) const {
    unsigned flagIndex  = valueType / entriesPerFlag;
    unsigned flagOffset = valueType % entriesPerFlag;
    Flags    mask       = 1ULL << flagOffset;

    return (activeFlags[flagIndex] & mask) != 0;
}


ActiveResources::ValueType ActiveResources::nextValidValueType(ActiveResources::ValueType startingValueType) const {
    unsigned flagIndex  = startingValueType / entriesPerFlag;
    unsigned flagOffset = startingValueType % entriesPerFlag;
    Flags    mask       = ~((1ULL << flagOffset) - 1);

    while (flagIndex < numberFlagElements && (activeFlags[flagIndex] & mask) == 0) {
        mask = static_cast<Flags>(-1);
        ++flagIndex;
    }

    ActiveResources::ValueType result;
    if (flagIndex >= numberFlagElements) {
        result = invalidValueType;
    } else {
        result = entriesPerFlag * flagIndex + lsbLocation64(activeFlags[flagIndex] & mask);
    }

    return result;
}


ActiveResources& ActiveResources::operator=(const ActiveResources& other) {
    currentCustomerId = other.currentCustomerId;
    std::memcpy(activeFlags, other.activeFlags, sizeof(activeFlags));

    return *this;
}


int ActiveResources::lsbLocation64(std::uint64_t value) {
    if (value & 0x00000000FFFFFFFFULL) {
        if (value & 0x000000000000FFFFULL) {
            if (value & 0x00000000000000FFULL) {
                if (value & 0x000000000000000FULL) {
                    if (value & 0x0000000000000003ULL) {
                        if (value & 0x0000000000000001ULL) {
                            return 0;
                        } else {
                            return 1;
                        }
                    } else {
                        if (value & 0x0000000000000004ULL) {
                            return 2;
                        } else {
                            return 3;
                        }
                    }
                } else {
                    if (value & 0x0000000000000030ULL) {
                        if (value & 0x0000000000000010ULL) {
                            return 4;
                        } else {
                            return 5;
                        }
                    } else {
                        if (value & 0x0000000000000040ULL) {
                            return 6;
                        } else {
                            return 7;
                        }
                    }
                }
            } else {
                if (value & 0x0000000000000F00ULL) {
                    if (value & 0x0000000000000300ULL) {
                        if (value & 0x0000000000000100ULL) {
                            return 8;
                        } else {
                            return 9;
                        }
                    } else {
                        if (value & 0x0000000000000400ULL) {
                            return 10;
                        } else {
                            return 11;
                        }
                    }
                } else {
                    if (value & 0x0000000000003000ULL) {
                        if (value & 0x0000000000001000ULL) {
                            return 12;
                        } else {
                            return 13;
                        }
                    } else {
                        if (value & 0x0000000000004000ULL) {
                            return 14;
                        } else {
                            return 15;
                        }
                    }
                }
            }
        } else {
            if (value & 0x0000000000FF0000ULL) {
                if (value & 0x00000000000F0000ULL) {
                    if (value & 0x0000000000030000ULL) {
                        if (value & 0x0000000000010000ULL) {
                            return 16;
                        } else {
                            return 17;
                        }
                    } else {
                        if (value & 0x0000000000040000ULL) {
                            return 18;
                        } else {
                            return 19;
                        }
                    }
                } else {
                    if (value & 0x0000000000300000ULL) {
                        if (value & 0x0000000000100000ULL) {
                            return 20;
                        } else {
                            return 21;
                        }
                    } else {
                        if (value & 0x0000000000400000ULL) {
                            return 22;
                        } else {
                            return 23;
                        }
                    }
                }
            } else {
                if (value & 0x000000000F000000ULL) {
                    if (value & 0x0000000003000000ULL) {
                        if (value & 0x0000000001000000ULL) {
                            return 24;
                        } else {
                            return 25;
                        }
                    } else {
                        if (value & 0x0000000004000000ULL) {
                            return 26;
                        } else {
                            return 27;
                        }
                    }
                } else {
                    if (value & 0x0000000030000000ULL) {
                        if (value & 0x0000000010000000ULL) {
                            return 28;
                        } else {
                            return 29;
                        }
                    } else {
                        if (value & 0x0000000040000000ULL) {
                            return 30;
                        } else {
                            return 31;
                        }
                    }
                }
            }
        }
    } else {
        if (value & 0x0000FFFF00000000ULL) {
            if (value & 0x000000FF00000000ULL) {
                if (value & 0x0000000F00000000ULL) {
                    if (value & 0x0000000300000000ULL) {
                        if (value & 0x0000000100000000ULL) {
                            return 32;
                        } else {
                            return 33;
                        }
                    } else {
                        if (value & 0x0000000400000000ULL) {
                            return 34;
                        } else {
                            return 35;
                        }
                    }
                } else {
                    if (value & 0x0000003000000000ULL) {
                        if (value & 0x0000001000000000ULL) {
                            return 36;
                        } else {
                            return 37;
                        }
                    } else {
                        if (value & 0x0000004000000000ULL) {
                            return 38;
                        } else {
                            return 39;
                        }
                    }
                }
            } else {
                if (value & 0x00000F0000000000ULL) {
                    if (value & 0x0000030000000000ULL) {
                        if (value & 0x0000010000000000ULL) {
                            return 40;
                        } else {
                            return 41;
                        }
                    } else {
                        if (value & 0x0000040000000000ULL) {
                            return 42;
                        } else {
                            return 43;
                        }
                    }
                } else {
                    if (value & 0x0000300000000000ULL) {
                        if (value & 0x0000100000000000ULL) {
                            return 44;
                        } else {
                            return 45;
                        }
                    } else {
                        if (value & 0x0000400000000000ULL) {
                            return 46;
                        } else {
                            return 47;
                        }
                    }
                }
            }
        } else {
            if (value & 0x00FF000000000000ULL) {
                if (value & 0x000F000000000000ULL) {
                    if (value & 0x0003000000000000ULL) {
                        if (value & 0x0001000000000000ULL) {
                            return 48;
                        } else {
                            return 49;
                        }
                    } else {
                        if (value & 0x0004000000000000ULL) {
                            return 50;
                        } else {
                            return 51;
                        }
                    }
                } else {
                    if (value & 0x0030000000000000ULL) {
                        if (value & 0x0010000000000000ULL) {
                            return 52;
                        } else {
                            return 53;
                        }
                    } else {
                        if (value & 0x0040000000000000ULL) {
                            return 54;
                        } else {
                            return 55;
                        }
                    }
                }
            } else {
                if (value & 0x0F00000000000000ULL) {
                    if (value & 0x0300000000000000ULL) {
                        if (value & 0x0100000000000000ULL) {
                            return 56;
                        } else {
                            return 57;
                        }
                    } else {
                        if (value & 0x0400000000000000ULL) {
                            return 58;
                        } else {
                            return 59;
                        }
                    }
                } else {
                    if (value & 0x3000000000000000ULL) {
                        if (value & 0x1000000000000000ULL) {
                            return 60;
                        } else {
                            return 61;
                        }
                    } else {
                        if (value & 0x4000000000000000ULL) {
                            return 62;
                        } else {
                            return value ? 63 : -1;
                        }
                    }
                }
            }
        }
    }

//    int result = 0;
//
//    if (value) {
//        unsigned      adjustment   = 32;
//        std::uint64_t runningValue = value;
//
//        while (adjustment) {
//            std::uint64_t mask = (1ULL << adjustment) - 1;
//            if ((runningValue & mask) == 0) {
//                runningValue >>= adjustment;
//                result += adjustment;
//            }
//
//            adjustment >>= 1;
//        }
//    } else {
//        result = -1;
//    }
//
//    return result;
}
