# DCCL IDs

*This section written by Claude*

lamss3 messages use public DCCL IDs from the GobySoft registry, so they can share a link with any other DCCL user. lamss3 holds two blocks:

| Block | Size | Encoded as |
|---|---|---|
| 20–23 | 4 | 1 byte |
| 5120–5375 | 256 | 2 bytes |

IDs below 128 cost one byte on the wire instead of two, so 20–23 are kept for short messages sent often over the acoustic link. Every other message takes the next free ID from 5120, whatever its toolbox. Every toolbox records its IDs here, and each toolbox's `lamss_<toolbox>_protobuf_dccl` test (`lamss_add_dccl_test`) fails if one of its messages' IDs is missing from this table or outside these blocks, or if a row names a message the toolbox does not have.

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
| 5124 | `lamss.autonomy.LAMSS_DEPLOY` | autonomy |
| 5125 | `lamss.autonomy.LAMSS_TRANSIT` | autonomy |
| 5126 | `lamss.autonomy.LAMSS_PROSECUTE` | autonomy |
| 5127 | `lamss.autonomy.LAMSS_STOVEPIPE` | autonomy |
| 5128 | `lamss.sonar.LAMSS_CONTACT` | sonar |
| 5129 | `lamss.sonar.LAMSS_TRACK` | sonar |
| 5130 | `lamss.sonar.BTR` | sonar |
| 5131 | `lamss.sonar.LAMSS_PROCESSING` | sonar |
| 5132 | `lamss.sonar.LAMSS_PINGER_COMMAND` | sonar |
| 5133 | `lamss.sonar.LAMSS_SIMULATE_TARGET` | sonar |
