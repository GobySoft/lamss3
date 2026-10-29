# DCCL IDs

*This section written by Claude*

lamss3 messages use public DCCL IDs from the GobySoft registry, so they can share a link with any other DCCL user. lamss3 holds two blocks:

| Block | Size | Encoded as |
|---|---|---|
| 20–23 | 4 | 1 byte |
| 5120–5375 | 256 | 2 bytes |

IDs below 128 cost one byte on the wire instead of two, so 20–23 are kept for short messages sent often over the acoustic link. Every other message takes the next free ID from 5120, whatever its toolbox. Every toolbox records its IDs here, and `lamss_core_protobuf_dccl` fails if a core message's ID is missing from this table or outside these blocks.

| ID | Message | Toolbox |
|---|---|---|
| 20 | `lamss.core.LAMSS_STATUS` | core |
| 21 | `lamss.core.LAMSS_STATUS_NMEA_SHIP` | core |
| 22 | `lamss.core.LAMSS_STATUS_NMEA_BUOY` | core |
| 23 | *free* | |
| 5120 | `lamss.core.VIEW_POINT` | core |
| 5121 | `lamss.core.VIEW_POLYGON` | core |
| 5122 | `lamss.core.VIEW_SEGLIST` | core |
| 5123 | `lamss.core.LAMSS_ENVIRONMENT_UPDATE` | core |
