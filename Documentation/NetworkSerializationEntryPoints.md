# Network Serialization Entry Point Inventory

Date: 2026-07-13

This inventory records the network serialization entry points that write, read, route, or apply game/network state. The current tree has mirrored base-game and D3XP implementations; paths under `neo/game` and `neo/d3xp` should be treated as parallel surfaces unless noted.

## Core Bit Message Primitives

| Entry point | Location | Role |
| --- | --- | --- |
| `idBitMsg` | `neo/idlib/BitMsg.h:44`, `neo/idlib/BitMsg.cpp:46` | Base bitstream reader/writer used by async, reliable messages, events, and snapshots. |
| `idBitMsg::WriteBits` / `ReadBits` | `neo/idlib/BitMsg.h:78`, `neo/idlib/BitMsg.h:107`, `neo/idlib/BitMsg.cpp:110`, `neo/idlib/BitMsg.cpp:362` | Lowest-level primitive serialization. |
| `idBitMsg::WriteDelta*` / `ReadDelta*` | `neo/idlib/BitMsg.h:93-131`, `neo/idlib/BitMsg.cpp:231-558` | Delta-encoded scalar and dictionary serialization. |
| `idBitMsg::WriteDeltaDict` / `ReadDeltaDict` | `neo/idlib/BitMsg.cpp:308`, `neo/idlib/BitMsg.cpp:558` | Dictionary delta serialization used for userinfo, serverinfo, synced CVars, and game metadata. |
| `idBitMsgDelta` | `neo/idlib/BitMsg.h:510`, `neo/idlib/BitMsg.cpp:648` | Snapshot delta layer that writes/reads against base and new-base messages. |
| `idBitMsgDelta::WriteDict` / `ReadDict` | `neo/idlib/BitMsg.cpp:823`, `neo/idlib/BitMsg.cpp:969` | Snapshot dictionary delta helpers. |

## Transport And Message Channel Layer

| Entry point | Location | Role |
| --- | --- | --- |
| `idMsgChannel::WriteMessageData` | `neo/framework/async/MsgChannel.cpp:333` | Serializes message payload/fragments into an outgoing channel packet. |
| `idMsgChannel::ReadMessageData` | `neo/framework/async/MsgChannel.cpp:366` | Reassembles/deserializes channel payload data. |
| `idMsgChannel::SendMessage` | `neo/framework/async/MsgChannel.cpp:471` | Sends serialized channel data through an `idPort`. |
| `idMsgChannel::Process` | `neo/framework/async/MsgChannel.cpp:539` | Parses incoming channel packets and sequences. |
| `idMsgChannel::SendReliableMessage` | `neo/framework/async/MsgChannel.cpp:670` | Queues reliable message payloads. |
| `idMsgChannel::GetReliableMessage` | `neo/framework/async/MsgChannel.cpp:690` | Pops deserialized reliable message payloads. |

## Async Client/Server Entry Points

| Entry point | Location | Role |
| --- | --- | --- |
| `idAsyncNetwork::WriteUserCmdDelta` | `neo/framework/async/AsyncNetwork.cpp:189` | Serializes delta-compressed `usercmd_t` input commands. |
| `idAsyncNetwork::ReadUserCmdDelta` | `neo/framework/async/AsyncNetwork.cpp:221` | Deserializes delta-compressed `usercmd_t` input commands. |
| `idAsyncClient::ProcessUnreliableServerMessage` | `neo/framework/async/AsyncClient.cpp:734` | Reads server unreliable messages and dispatches game snapshots/messages. |
| `idAsyncClient::ProcessReliableMessagePure` | `neo/framework/async/AsyncClient.cpp:877` | Reads pure-server reliable messages. |
| `idAsyncClient::ProcessMessage` | `neo/framework/async/AsyncClient.cpp:1612` | Top-level client packet/message parser. |
| `idAsyncClient::SendReliableGameMessage` | `neo/framework/async/AsyncClient.cpp:1722` | Wraps and sends game reliable messages from client to server. |
| `idAsyncServer::ProcessUnreliableClientMessage` | `neo/framework/async/AsyncServer.cpp:1224` | Reads client unreliable messages, including usercmd/userinfo updates. |
| `idAsyncServer::ProcessMessage` | `neo/framework/async/AsyncServer.cpp:2198` | Top-level server packet/message parser. |
| `idAsyncServer::SendReliableGameMessage` | `neo/framework/async/AsyncServer.cpp:2263` | Wraps and sends reliable game messages to one client. |
| `idAsyncServer::SendReliableGameMessageExcluding` | `neo/framework/async/AsyncServer.cpp:2292` | Sends reliable game messages to all but one client. |
| `idAsyncServer::LocalClientSendReliableMessage` | `neo/framework/async/AsyncServer.cpp:2319` | Local-client reliable message path. |
| `idNetworkSystem::ServerSendReliableMessage` | `neo/framework/async/NetworkSystem.cpp:43` | Game-facing facade into server reliable messaging. |
| `idNetworkSystem::ServerSendReliableMessageExcluding` | `neo/framework/async/NetworkSystem.cpp:54` | Game-facing broadcast-minus-client facade. |
| `idNetworkSystem::ClientSendReliableMessage` | `neo/framework/async/NetworkSystem.cpp:149` | Game-facing client reliable message facade. |

## Game Snapshot Entry Points

| Entry point | Base game | D3XP | Role |
| --- | --- | --- | --- |
| `idGameLocal::ServerWriteInitialReliableMessages` | `neo/game/Game_network.cpp:378` | `neo/d3xp/Game_network.cpp:378` | Writes initial reliable state for a joining client. |
| `idGameLocal::WriteGameStateToSnapshot` | `neo/game/Game_network.cpp:518` | `neo/d3xp/Game_network.cpp:518` | Serializes global game state and multiplayer state into the snapshot delta. |
| `idGameLocal::ReadGameStateFromSnapshot` | `neo/game/Game_network.cpp:533` | `neo/d3xp/Game_network.cpp:533` | Deserializes global game state from a snapshot delta. |
| `idGameLocal::ServerWriteSnapshot` | `neo/game/Game_network.cpp:550` | `neo/d3xp/Game_network.cpp:550` | Main server snapshot serializer; iterates entities, player state, and game state. |
| `idGameLocal::ServerApplySnapshot` | `neo/game/Game_network.cpp:694` | `neo/d3xp/Game_network.cpp:708` | Applies a server-side stored snapshot by sequence. |
| `idGameLocal::ClientReadSnapshot` | `neo/game/Game_network.cpp:957` | `neo/d3xp/Game_network.cpp:971` | Main client snapshot deserializer; spawns/recycles entities and applies entity/player/game state. |
| `idGameLocal::ClientApplySnapshot` | `neo/game/Game_network.cpp:1242` | `neo/d3xp/Game_network.cpp:1274` | Applies a client-side stored snapshot by sequence. |
| `idGameLocal::ApplySnapshot` | `neo/game/Game_network.cpp:482` | `neo/d3xp/Game_network.cpp:482` | Common snapshot sequence lookup/apply helper. |

## Entity And Gameplay Snapshot Entry Points

| Entry point family | Base game | D3XP | Notes |
| --- | --- | --- | --- |
| `idEntity::WriteToSnapshot` / `ReadFromSnapshot` | `neo/game/Entity.cpp:4704`, `neo/game/Entity.cpp:4712` | `neo/d3xp/Entity.cpp:4878`, `neo/d3xp/Entity.cpp:4886` | Root entity snapshot override point. |
| `idEntity::WriteBindToSnapshot` / `ReadBindFromSnapshot` | `neo/game/Entity.cpp:4582`, `neo/game/Entity.cpp:4606` | `neo/d3xp/Entity.cpp:4756`, `neo/d3xp/Entity.cpp:4780` | Bind/master relationship state. |
| `idEntity::WriteColorToSnapshot` / `ReadColorFromSnapshot` | `neo/game/Entity.cpp:4643`, `neo/game/Entity.cpp:4658` | `neo/d3xp/Entity.cpp:4817`, `neo/d3xp/Entity.cpp:4832` | Entity color/shader parm state. |
| `idEntity::WriteGUIToSnapshot` / `ReadGUIFromSnapshot` | `neo/game/Entity.cpp:4673`, `neo/game/Entity.cpp:4687` | `neo/d3xp/Entity.cpp:4847`, `neo/d3xp/Entity.cpp:4861` | GUI state. |
| `idPlayer::WriteToSnapshot` / `ReadFromSnapshot` | `neo/game/Player.cpp:8006`, `neo/game/Player.cpp:8031` | `neo/d3xp/Player.cpp:9527`, `neo/d3xp/Player.cpp:9559` | Player entity and physics snapshot state. |
| `idMultiplayerGame::WriteToSnapshot` / `ReadFromSnapshot` | `neo/game/MultiplayerGame.cpp:2102`, `neo/game/MultiplayerGame.cpp:2128` | `neo/d3xp/MultiplayerGame.cpp:2686`, `neo/d3xp/MultiplayerGame.cpp:2719` | Global multiplayer score/game-state snapshot. |
| Physics `WriteToSnapshot` / `ReadFromSnapshot` | `neo/game/physics/*` | `neo/d3xp/physics/*` | Physics base, actor, player, monster, parametric, rigid body, static, static multi, and AF snapshot serializers. |
| Gameplay overrides | `Item`, `Light`, `Misc`, `Moveable`, `Mover`, `Projectile`, `Weapon`, `Fx`, `BrittleFracture` | Same plus D3XP variants | Per-entity subclass state. Search `WriteToSnapshot` and `ReadFromSnapshot` under each module. |

## Network Event Entry Points

| Entry point | Base game | D3XP | Role |
| --- | --- | --- | --- |
| `idEntity::ServerSendEvent` | `neo/game/Entity.cpp:4723` | `neo/d3xp/Entity.cpp:4897` | Server-side event serialization and reliable/unreliable event queue insertion. |
| `idEntity::ClientReceiveEvent` | `neo/game/Entity.cpp:4814` | `neo/d3xp/Entity.cpp:4988` | Root client event deserializer. |
| `idAnimatedEntity::ClientReceiveEvent` | `neo/game/Entity.cpp:5265` | `neo/d3xp/Entity.cpp:5477` | Animation/event subclass handling. |
| Subclass `ClientReceiveEvent` overrides | `BrittleFracture`, `Item`, `Light`, `Misc`, `Moveable`, `Player`, `Projectile`, `Weapon` | Same plus D3XP variants | Read event-specific `idBitMsg` payloads generated by `ServerSendEvent` call sites. |

## Reliable Game Message Payloads

| Surface | Base game | D3XP | Payload examples |
| --- | --- | --- | --- |
| `idMultiplayerGame` reliable messages | `neo/game/MultiplayerGame.cpp` | `neo/d3xp/MultiplayerGame.cpp` | Restart, warmup time, tournament line, menu state, sound event, database event, drop weapon, vote, chat, voice chat, start state, server info. |
| `GAME_RELIABLE_MESSAGE_*` writes | Base: lines around `984`, `1050`, `1148`, `1862`, `2099-3371` | D3XP: lines around `1413`, `1492`, `1607`, `2422`, `2683-4104` | Main enum-tagged game message serialization points. |

## Modernization Notes

- `idBitMsg` and `idBitMsgDelta` are the serialization contract. Changes there affect snapshots, reliable messages, channel fragmentation, cvar/userinfo deltas, and game events.
- Base game and D3XP copies are structurally parallel but line numbers diverge. Serializer changes should usually be made in both trees.
- `ServerWriteSnapshot` and `ClientReadSnapshot` are the highest-value integration tests for a network serialization refactor.
