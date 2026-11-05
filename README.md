# VJ_2026-2027_Pocket-Bomberman

## 指定草丛掉落的道具

在 `Pocket-Bomberman/levels/levelXX.txt` 中添加：

```text
POWERUP X Y TYPE
```

`X Y` 是草丛在**裁剪后的地图**中的格子坐标，从左上角 `(0, 0)` 开始，X 向右、Y 向下，与 `PLAYER` 和敌人配置使用同一套坐标。

| TYPE | 道具效果 |
| --- | --- |
| `BOMBUP` | 增加同时放置的炸弹数量 |
| `FIREUP` | 增加爆炸范围 |
| `HEART` | 抵挡一次伤害 |
| `SPEEDUP` | 提高移动速度 |

例如，当前第一关的这些位置都是草丛，可以按需加入 `level01.txt`：

```text
POWERUP 1 1 BOMBUP
POWERUP 14 2 FIREUP
POWERUP 14 4 HEART
POWERUP 1 9 SPEEDUP
```

- 指定草丛被炸毁时，必定在原地生成指定道具，每次加载关卡只掉落一次。
- 未配置的草丛不掉落道具；不再使用随机概率或随机道具池。
- 同一坐标写多条配置时，最后一条生效。
- 死亡重试会恢复本关的固定掉落；切关时会清除上一关的配置。
- 格式错误、未知道具类型，或坐标不在草丛上时，关卡加载失败并输出原因。
- `#` 开头的行是注释；第一关文件中提供了一条可取消注释的示例。
