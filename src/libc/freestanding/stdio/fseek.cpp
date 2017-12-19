#include <stdio.h>
#include "file.h"
#include <system_calls.h>

enum class Origin {
    Current = SEEK_CUR,
    End = SEEK_END,
    Set = SEEK_SET
};

int fseek(FILE* stream, long int offset, int whence) {
    auto result = seekSynchronous(stream->descriptor, offset, whence);

    if (result.success) {
        
        switch(static_cast<Origin>(whence)) {
            case Origin::Current: {
                stream->position += offset;
                break;
            }
            case Origin::End: {
                stream->position = stream->length - offset;
                break;
            }
            case Origin::Set: {
                stream->position = offset;
                break;
            }
        }

        return 0;
    }
    else {
        return -1;
    }

}