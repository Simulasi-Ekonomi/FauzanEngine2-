#include "RHI.h"

RHI* GRHI = nullptr;

RHI& RHI::Get() {
    return *GRHI;
}
