#include "src/limbo/game_canvas.hpp"
#include "src/limbo/iso_transform.hpp"

#include "src/limbo/string_table.hpp"

void GameCanvas::initialize_level_scene() {
    SharedArray<int32_t> nArray{0, 0};
    SharedArray<int32_t> nArray2{0, 0};
    int32_t n = (state().tile_width >> 1);
    int32_t n2 = 0;
    int32_t n3 = 0;
    this->scene_dirty = true;
    for (n2 = 0; n2 < state().level_rows; ++n2) {
        for (n3 = 0; n3 < state().level_cols; ++n3) {
            IsoTransform::world_to_screen(nArray, nArray2);
            this->tile_screen_positions[n2 * GameCanvas::state().level_cols + 0 * this->level_tile_count + n3] = (int16_t)(nArray2[0] - n);
            this->tile_screen_positions[n2 * GameCanvas::state().level_cols + 1 * this->level_tile_count + n3] = (int16_t)nArray2[1];
            nArray[1] = nArray[1] + this->tile_world_step;
        }
        nArray[0] = nArray[0] + this->tile_world_step;
        nArray[1] = 0;
    }
    for (n2 = 1; n2 < 25; ++n2) {
        GameCanvas::state().actors[n2] = nullptr;
    }
    if (state().actors[0] != nullptr) {
        ActorSystem::reset_actor(state().actors[0]);
        ActorSystem::update_footprint_tiles(state().actors[0]);
    }
}

void GameCanvas::carve_floor_tile(const SharedArray<int8_t> &byArray, int32_t n, int32_t n2, int32_t n3) {
    int32_t n4 = n * state().level_cols + n2;
    if (n4 >= state().tile_walkable.length() || n4 < 0) {
        return;
    }
    byArray[n4] = (int8_t)n3;
    GameCanvas::state().tile_walkable[n4] = 0;
}

void GameCanvas::carve_corridor(const SharedArray<int8_t> &byArray, const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2, int32_t n, int32_t n2,
          const SharedArray<int32_t> &nArray3) {
    {
        int32_t n3;
        int32_t n4;
        SharedArray<int32_t> nArray4;
        {
            bool bl = false;
            bool bl2 = false;
            nArray4 = SharedArray<int32_t>{nArray[0], nArray[1]};
            n4 = n2;
            n3 = 0;
            int32_t n5 = 0;
            int32_t n6 = 0;
            while (!bl2) {
                {
                    if (state().random->nextInt() % nArray3[16] == 0 &&
                        (this->var_byte_s = (int8_t)(this->var_byte_s + 1)) < nArray3[15]) {
                        while ((n5 = wrappingAbs(state().random->nextInt()) % 4 + 1) == n4) {
                        }
                        n6 = wrappingAbs(state().random->nextInt() % min32(state().level_rows, state().level_cols));
                        if (n5 == 2) {
                            this->carve_corridor(byArray, nArray4, SharedArray<int32_t>{n6, 3}, n, n5, nArray3);
                        } else if (n5 == 1) {
                            this->carve_corridor(byArray, nArray4, SharedArray<int32_t>{n6, state().level_cols - 3}, n, n5, nArray3);
                        } else if (n5 == 3) {
                            this->carve_corridor(byArray, nArray4, SharedArray<int32_t>{state().level_rows - 3, n6}, n, n5, nArray3);
                        } else if (n5 == 4) {
                            this->carve_corridor(byArray, nArray4, SharedArray<int32_t>{3, n6}, n, n5, nArray3);
                        }
                    }
                    if (n4 == 1 || n4 == 2) {
                        bl = state().random->nextInt() % nArray3[19] == 0;
                        this->var_byte_u = (int8_t)(this->var_byte_u + 1);
                        for (n3 = 0; n3 < n; ++n3) {
                            this->carve_floor_tile(byArray, nArray4[0] + n3, nArray4[1], nArray3[4]);
                        }
                        if (bl && this->var_byte_t < 50 && this->var_byte_u > 10) {
                            int8_t by = this->var_byte_t;
                            this->var_byte_t = (int8_t)(by + 1);
                            this->var_int_arr_m[by] = nArray4[0] + 1;
                            int8_t by2 = this->var_byte_t;
                            this->var_byte_t = (int8_t)(by2 + 1);
                            this->var_int_arr_m[by2] = nArray4[1];
                        }
                    } else if (n4 == 3 || n4 == 4) {
                        bl = state().random->nextInt() % nArray3[19] == 0;
                        this->var_byte_u = (int8_t)(this->var_byte_u + 1);
                        for (n3 = 0; n3 < n; ++n3) {
                            this->carve_floor_tile(byArray, nArray4[0], nArray4[1] + n3, nArray3[4]);
                        }
                        if (bl && this->var_byte_t < 50 && this->var_byte_u > 10) {
                            int8_t by = this->var_byte_t;
                            this->var_byte_t = (int8_t)(by + 1);
                            this->var_int_arr_m[by] = nArray4[0];
                            int8_t by3 = this->var_byte_t;
                            this->var_byte_t = (int8_t)(by3 + 1);
                            this->var_int_arr_m[by3] = nArray4[1] + 1;
                        }
                    }
                    do {
                        n5 = wrappingAbs(state().random->nextInt() % 4);
                        if (nArray2[0] < nArray4[0]) {
                            if (n5 == 1) {
                                n4 = 2;
                            }
                            if (n5 == 2) {
                                n4 = 1;
                            }
                            if (n5 == 3) {
                                n4 = 4;
                            }
                        } else if (nArray2[0] > nArray4[0]) {
                            if (n5 == 1) {
                                n4 = 2;
                            }
                            if (n5 == 2) {
                                n4 = 1;
                            }
                            if (n5 == 3) {
                                n4 = 3;
                            }
                        } else {
                            n4 = n5 < 2 ? 2 : 1;
                        }
                        if (n4 == 1 && nArray4[1] < nArray2[1]) {
                            nArray4[1] = nArray4[1] + 1;
                        } else if (n4 == 3 && nArray4[0] < nArray2[0]) {
                            nArray4[0] = nArray4[0] + 1;
                        } else {
                            if (n4 != 2 || nArray4[1] <= nArray2[1]) continue;
                            nArray4[1] = nArray4[1] - 1;
                        }
                        goto end_block34;
                    } while (n4 != 4 || nArray4[0] <= nArray2[0]);
                    nArray4[0] = nArray4[0] - 1;
                }
            end_block34:
                bl2 = nArray4[0] == nArray2[0] && nArray4[1] == nArray2[1];
            }
            if (n4 != 1 && n4 != 2) goto end_block35;
            this->var_byte_u = (int8_t)(this->var_byte_u + 1);
            for (n3 = 0; n3 < n; ++n3) {
                this->carve_floor_tile(byArray, nArray4[0] + n3, nArray4[1], nArray3[4]);
            }
            goto end_block36;
        }
    end_block35:
        if (n4 != 3 && n4 != 4) goto end_block36;
        this->var_byte_u = (int8_t)(this->var_byte_u + 1);
        for (n3 = 0; n3 < n; ++n3) {
            this->carve_floor_tile(byArray, nArray4[0], nArray4[1] + n3, nArray3[4]);
        }
    }
end_block36:;
}

void GameCanvas::build_wall_layer(const SharedArray<int8_t> &byArray, const SharedArray<int8_t> &byArray2, const SharedArray<int32_t> &nArray) {
    int32_t n = 0;
    int32_t n2 = 0;
    for (n = 0; n < state().level_rows; ++n) {
        for (n2 = 0; n2 < state().level_cols; ++n2) {
            if (n * state().level_cols + n2 > byArray.length() - 1 || n * state().level_cols + n2 < 0 ||
                n * state().level_cols + (n2 - 1) > byArray.length() - 1 || n * state().level_cols + (n2 - 1) < 0 ||
                n * state().level_cols + (n2 + 1) > byArray.length() - 1 || n * state().level_cols + (n2 + 1) < 0 ||
                (n - 1) * state().level_cols + n2 > byArray.length() - 1 || (n - 1) * state().level_cols + n2 < 0 ||
                (n + 1) * state().level_cols + n2 > byArray.length() - 1 || (n + 1) * state().level_cols + n2 < 0 ||
                byArray[n * state().level_cols + n2] != nArray[4])
                continue;
            if (byArray[n * state().level_cols + (n2 - 1)] == nArray[3]) {
                if (byArray[(n - 1) * state().level_cols + n2] == nArray[3]) {
                    byArray2[n * GameCanvas::state().level_cols + n2] = (int8_t)nArray[9];
                    continue;
                }
                if (byArray[(n + 1) * state().level_cols + n2] == nArray[3]) {
                    byArray2[n * GameCanvas::state().level_cols + n2] = (int8_t)nArray[10];
                    continue;
                }
                byArray2[n * GameCanvas::state().level_cols + n2] = (int8_t)nArray[5];
                continue;
            }
            if (byArray[n * state().level_cols + (n2 + 1)] == nArray[3]) {
                if (byArray[(n - 1) * state().level_cols + n2] == nArray[3]) {
                    byArray2[n * GameCanvas::state().level_cols + n2] = (int8_t)nArray[11];
                    continue;
                }
                if (byArray[(n + 1) * state().level_cols + n2] == nArray[3]) {
                    byArray2[n * GameCanvas::state().level_cols + n2] = (int8_t)nArray[12];
                    continue;
                }
                byArray2[n * GameCanvas::state().level_cols + n2] = (int8_t)nArray[6];
                continue;
            }
            if (byArray[(n + 1) * state().level_cols + n2] == nArray[3]) {
                byArray2[n * GameCanvas::state().level_cols + n2] = (int8_t)nArray[7];
                continue;
            }
            if (byArray[(n - 1) * state().level_cols + n2] != nArray[3]) continue;
            byArray2[n * GameCanvas::state().level_cols + n2] = (int8_t)nArray[8];
        }
    }
}

void GameCanvas::carve_corridor_endpoints(const SharedArray<int8_t> &byArray, const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2,
          const SharedArray<int32_t> &nArray3) {
    int32_t n;
    int32_t n2;
    int32_t n3 = byArray.length();
    int8_t by = state().level_cols;
    int32_t n4 = 0;
    SharedArray<SharedArray<int8_t>> byArrayArray{SharedArray<int8_t>{-1, 1},  SharedArray<int8_t>{-1, 0}, SharedArray<int8_t>{-1, -1},
                                       SharedArray<int8_t>{0, 1},   SharedArray<int8_t>{0, 0},  SharedArray<int8_t>{0, -1},
                                       SharedArray<int8_t>{1, 1},   SharedArray<int8_t>{1, 0},  SharedArray<int8_t>{1, -1}};
    int32_t n5 = byArrayArray.length();
    for (n4 = 0; n4 < n5; ++n4) {
        n2 = nArray[0] + byArrayArray[n4][0];
        n = nArray[1] + byArrayArray[n4][1];
        if (n2 * by + n >= n3 || n2 * by + n < 0) continue;
        byArray[n2 * by + n] = (int8_t)nArray3[4];
        GameCanvas::state().tile_walkable[n2 * by + n] = 0;
    }
    for (n4 = 0; n4 < n5; ++n4) {
        n2 = nArray2[0] + byArrayArray[n4][0];
        n = nArray2[1] + byArrayArray[n4][1];
        if (n2 * by + n >= n3 || n2 * by + n < 0) continue;
        byArray[n2 * by + n] = (int8_t)nArray3[4];
    }
}

void GameCanvas::generate_level(const SharedArray<int32_t> &nArray, const SharedArray<int32_t> &nArray2, int32_t n, int32_t n2) {
    SharedArray<int8_t> byArray;
    int32_t n3 = 0;
    int32_t n4 = 0;
    int32_t n5 = 0;
    int8_t by = 0;
    SharedArray<int32_t> nArray3;
    SharedArray<int32_t> nArray4;
    state().tile_walkable.setNull();
    this->tile_enter_script.setNull();
    this->tile_exit_script.setNull();
    state().tile_use_script.setNull();
    this->tile_screen_positions.setNull();
    for (n4 = 1; n4 < state().actors.length(); ++n4) {
        GameCanvas::state().actors[n4] = nullptr;
    }
    if (this->player != nullptr) {
        this->player->summoned_actor = nullptr;
    }
    this->var_byte_s = 0;
    this->var_byte_t = 0;
    this->var_byte_u = 0;
    state().world_item_count = 0;
    state().tile_layers.clear();
    state().level_rows = (int8_t)nArray[1];
    state().level_cols = (int8_t)nArray[2];
    this->level_tile_count = (int16_t)(state().level_rows * state().level_cols);
    state().tile_walkable = SharedArray<int8_t>(this->level_tile_count);
    this->tile_enter_script = SharedArray<int8_t>(this->level_tile_count);
    this->tile_exit_script = SharedArray<int8_t>(this->level_tile_count);
    state().tile_use_script = SharedArray<int8_t>(this->level_tile_count);
    this->tile_screen_positions = SharedArray<int16_t>(this->level_tile_count * 2);
    for (n4 = 0; n4 < state().level_rows; ++n4) {
        for (n5 = 0; n5 < state().level_cols; ++n5) {
            this->tile_enter_script[n4 * GameCanvas::state().level_cols + n5] = -1;
            this->tile_exit_script[n4 * GameCanvas::state().level_cols + n5] = -1;
            GameCanvas::state().tile_use_script[n4 * GameCanvas::state().level_cols + n5] = -1;
        }
    }
    byArray = SharedArray<int8_t>(this->level_tile_count);
    state().tile_layers.push_back(byArray);
    for (n4 = 0; n4 < state().level_rows; ++n4) {
        for (n5 = 0; n5 < state().level_cols; ++n5) {
            byArray[n4 * GameCanvas::state().level_cols + n5] = (int8_t)nArray[3];
            GameCanvas::state().tile_walkable[n4 * GameCanvas::state().level_cols + n5] = 1;
        }
    }
    nArray3 = SharedArray<int32_t>{2, 2};
    nArray4 = SharedArray<int32_t>{state().level_rows - 2, state().level_cols - 2};
    this->carve_corridor(byArray, nArray3, nArray4, nArray[14], 1, nArray);
    this->carve_corridor_endpoints(byArray, nArray3, nArray4, nArray);
    SharedArray<int8_t> byArray2 = byArray;
    byArray = SharedArray<int8_t>(this->level_tile_count);
    state().tile_layers.push_back(byArray);
    this->build_wall_layer(byArray2, byArray, nArray);
    byArray = SharedArray<int8_t>(this->level_tile_count);
    state().tile_layers.push_back(byArray);
    byArray[nArray3[0] * GameCanvas::state().level_cols + nArray3[1]] = (int8_t)nArray[13];
    byArray[nArray4[0] * GameCanvas::state().level_cols + nArray4[1]] = (int8_t)nArray[13];
    this->tile_enter_script[nArray4[0] * GameCanvas::state().level_cols + 0 * this->level_tile_count + nArray4[1]] = (int8_t)n2;
    this->tile_exit_script[nArray3[0] * GameCanvas::state().level_cols + nArray3[1]] = -2;
    this->tile_enter_script[nArray3[0] * GameCanvas::state().level_cols + nArray3[1]] = -2;
    GameCanvas::state().tile_use_script[nArray3[0] * GameCanvas::state().level_cols + nArray3[1]] = (int8_t)n;
    this->initialize_level_scene();
    for (n4 = 0; n4 < this->var_byte_t && n4 < nArray[18]; n4 += 2) {
        if (nArray2[n3] != 0) {
            this->place_world_item(nArray2[n3], false, this->var_int_arr_m[n4], this->var_int_arr_m[n4 + 1]);
            ++n3;
        }
        by = 2;
        while (state().actors[by] != nullptr) {
            by = (int8_t)(by + 1);
        }
        this->spawn_actor_in_slot(std::nullopt, state().game_data->get_string(GameCanvas::state().game_data->script_table[nArray[17]][1]), by,
                shiftLeft32(this->var_int_arr_m[n4], 7), shiftLeft32(this->var_int_arr_m[n4 + 1], 7),
                GameCanvas::state().game_data->script_table[nArray[17]]);
    }
}

void GameCanvas::clear_tile_layers() {
    state().tile_layers.clear();
    this->scene_dirty = true;
    if (state().actors[0] != nullptr) {
        ActorSystem::reset_actor(state().actors[0]);
        ActorSystem::update_footprint_tiles(state().actors[0]);
    }
}

namespace {

void decode_rle_tile_layer(
    const SharedArray<int8_t> &buffer,
    int32_t &offset,
    int32_t buffer_length,
    int32_t rows,
    int32_t cols,
    SharedArray<int8_t> &dest) {
    const int32_t total_tiles = rows * cols;
    int32_t row = 0;
    int32_t col = 0;
    int32_t tiles_written = 0;

    while (tiles_written < total_tiles && offset < buffer_length) {
        const uint8_t byte = static_cast<uint8_t>(buffer[offset++]);
        int32_t count = 1;
        int8_t value = static_cast<int8_t>(byte);

        if (byte == 255) {
            if (offset + 1 >= buffer_length) {
                break;
            }
            count = static_cast<uint8_t>(buffer[offset++]);
            value = buffer[offset++];
        }

        for (int32_t i = 0; i < count && tiles_written < total_tiles; ++i) {
            dest[row * cols + col] = value;
            ++tiles_written;
            if (++row == rows) {
                row = 0;
                ++col;
            }
        }
    }
}

}

void GameCanvas::load_level_map(const std::string &string) {
    state().tile_walkable.setNull();
    this->tile_enter_script.setNull();
    this->tile_exit_script.setNull();
    state().tile_use_script.setNull();
    this->tile_screen_positions.setNull();

    const int32_t resource_size = GameCanvas::load_resource(string);
    if (resource_size < 2) {
        current_string_table().resource_buffer.setNull();
        this->initialize_level_scene();
        return;
    }

    if (this->player != nullptr) {
        this->player->summoned_actor = nullptr;
    }
    state().tile_layers.clear();
    MagicEffects::clear_all();

    const SharedArray<int8_t> &buffer = current_string_table().resource_buffer;
    state().level_rows = buffer[0];
    state().level_cols = buffer[1];
    this->level_tile_count = static_cast<int16_t>(state().level_rows * state().level_cols);
    state().world_item_count = 0;

    state().tile_walkable = SharedArray<int8_t>(this->level_tile_count);
    this->tile_enter_script = SharedArray<int8_t>(this->level_tile_count);
    this->tile_exit_script = SharedArray<int8_t>(this->level_tile_count);
    state().tile_use_script = SharedArray<int8_t>(this->level_tile_count);
    this->tile_screen_positions = SharedArray<int16_t>(this->level_tile_count * 2);

    for (int32_t i = 0; i < this->level_tile_count; ++i) {
        this->tile_enter_script[i] = -1;
        this->tile_exit_script[i] = -1;
        GameCanvas::state().tile_use_script[i] = -1;
    }

    int32_t offset = 2;
    decode_rle_tile_layer(buffer, offset, resource_size, state().level_rows, state().level_cols, state().tile_walkable);

    while (offset < resource_size) {
        SharedArray<int8_t> layer(this->level_tile_count);
        decode_rle_tile_layer(buffer, offset, resource_size, state().level_rows, state().level_cols, layer);
        state().tile_layers.push_back(layer);
    }

    current_string_table().resource_buffer.setNull();
    this->initialize_level_scene();
}

void GameCanvas::redraw_tile_background(Graphics *graphics) {
    (void)graphics;
    SharedArray<int8_t> byArray;
    int32_t n = 0;
    int32_t n2 = 0;
    int32_t n3 = 0;
    int32_t n4 = 0;
    int32_t n5 = 0;
    int32_t n6 = 0;
    if (!this->scene_dirty) {
        return;
    }
    this->scene_dirty = false;
    this->scene_graphics->setColor(state().scene_clear_color);
    this->scene_graphics->fillRect(0, 0, (int32_t)state().canvas_width, (int32_t)state().canvas_height);
    for (n6 = 0; n6 < (int32_t)state().tile_layers.size() - 1; ++n6) {
        byArray = state().tile_layers[(size_t)n6];
        for (n4 = this->visible_tile_min[0]; n4 <= this->visible_tile_max[0] && n4 < state().level_rows; ++n4) {
            for (n5 = this->visible_tile_min[1]; n5 <= this->visible_tile_max[1] && n5 < state().level_cols; ++n5) {
                if (n4 < 0 || n5 < 0) continue;
                n2 = this->tile_screen_positions[n4 * state().level_cols + 0 * this->level_tile_count + n5] + this->camera_offset[0];
                n3 = this->tile_screen_positions[n4 * state().level_cols + 1 * this->level_tile_count + n5] + this->camera_offset[1];
                if (byArray[n4 * state().level_cols + n5] != 0) {
                    n = SpriteAtlas::frame_height(this->tile_atlas, byArray[n4 * state().level_cols + n5]);
                }
                if (n2 <= -state().tile_width || n2 >= state().canvas_width || n3 <= -state().tile_height || n3 >= state().canvas_height + n ||
                    byArray[n4 * state().level_cols + n5] == 0)
                    continue;
                SpriteAtlas::draw_frame(this->scene_graphics, this->tile_atlas, byArray[n4 * state().level_cols + n5], n2,
                     n3);
            }
        }
    }
}


Actor *GameCanvas::spawn_actor(const std::string &string, int32_t n, int32_t n2, const SharedArray<int32_t> &nArray) {
    int8_t by = 0;
    for (by = (int8_t)(state().actors.length() - 1); by >= 0 && state().actors[by] != nullptr; by = (int8_t)(by - 1)) {
    }
    return this->spawn_actor_in_slot(std::nullopt, string, by, n, n2, nArray);
}

Actor *GameCanvas::spawn_actor_in_slot(const std::optional<std::string> &string, const std::string &string2, int8_t by, int32_t n, int32_t n2, const SharedArray<int32_t> &nArray) {
    if (by < 0 || by >= 25) {
        return nullptr;
    }
    if (by == 0 && this->player != nullptr) {
        GameCanvas::state().actors[by] = this->player;
        ActorSystem::reset_actor(this->player);
    } else {
        GameCanvas::state().actors[by] = ActorSystem::create_actor(string2, (int8_t)(by + 1));
        if (by == 0) {
            ActorSystem::apply_class(state().actors[by], (int8_t)(state().menu_selection[1] + 1), false);
        }
        ActorSystem::apply_definition(state().actors[by], nArray);
        GameCanvas::state().actors[by]->name = string;
    }
    ActorSystem::set_position(state().actors[by], n, n2);
    if (by == 0) {
        this->player = state().actors[by];
        this->player->collision_enabled = (int8_t)(state().var_boolean_c ? 1 : 0);
        this->player->loot_drop_enabled = 0;
        this->player->death_script_id = (int8_t)-1;
        this->scene_dirty = true;
        this->camera_actor_slot = by;
        this->camera_offset[0] = this->canvas_center_x - GameCanvas::state().actors[this->camera_actor_slot]->screen_position[0];
        this->camera_offset[1] = this->canvas_center_y - GameCanvas::state().actors[this->camera_actor_slot]->screen_position[1];
    }
    this->highest_actor_slot = max32(this->highest_actor_slot, by);
    return state().actors[by];
}

void GameCanvas::player_died() {
    MagicEffects::clear_all();
    GameCanvas::set_game_state((int8_t)11);
    ActorSystem::reset_actor(GameCanvas::state().instance->player);
    ActorSystem::set_position(GameCanvas::state().instance->player, (int32_t)state().respawn_x, (int32_t)state().respawn_y);
    GameCanvas::set_status_message(std::nullopt, 0, 0, 0);
}

void GameCanvas::remove_actor(int32_t n) {
    if (state().actors[n] == nullptr) {
        return;
    }
    if (n == 0) {
        MagicEffects::clear_all();
        GameCanvas::set_game_state((int8_t)11);
        ActorSystem::reset_actor(GameCanvas::state().instance->player);
        ActorSystem::set_position(GameCanvas::state().instance->player, (int32_t)state().respawn_x, (int32_t)state().respawn_y);
        GameCanvas::set_status_message(std::nullopt, 0, 0, 0);
        return;
    }
    if (n == GameCanvas::state().instance->camera_actor_slot) {
        GameCanvas::set_speaker_name(std::nullopt);
    }
    GameCanvas::state().actors[n] = nullptr;
    if (n == GameCanvas::state().instance->highest_actor_slot) {
        while (n > 0 && state().actors[n] == nullptr) {
            --n;
        }
        GameCanvas::state().instance->highest_actor_slot = n;
    }
}

void GameCanvas::set_player_input_enabled(bool bl) {
    this->player_input_enabled = bl;
    if (bl) {
        clear_pending_key();
    }
}

void GameCanvas::load_tile_atlas(const std::string &string) { this->tile_atlas = SpriteAtlas::load_atlas(string); }

void GameCanvas::show_splash_screen(int32_t n, int32_t n2) {
    this->splash_frame = n;
    this->splash_color = n2;
    if (n == 4) {
        GameCanvas::set_game_state((int8_t)21);
        return;
    }
    GameCanvas::set_game_state((int8_t)8);
}

void GameCanvas::set_tile_blocked(int32_t n, int32_t n2, bool bl) { GameCanvas::state().tile_walkable[n * GameCanvas::state().level_cols + n2] = (int8_t)(bl ? 1 : 0); }

void GameCanvas::set_tile_frame(int32_t n, int32_t n2, int32_t n3, int32_t n4) {
    GameCanvas::state().tile_layers[(size_t)n3][n * GameCanvas::state().level_cols + n2] = (int8_t)n4;
    this->scene_dirty = true;
}

void GameCanvas::center_camera_at(int32_t n, int32_t n2) {
    SharedArray<int32_t> nArray{0, 0};
    SharedArray<int32_t> nArray2{n, n2};
    IsoTransform::world_to_screen(nArray2, nArray);
    this->camera_offset[0] = this->canvas_center_x - nArray[0];
    this->camera_offset[1] = this->canvas_center_y - nArray[1];
    this->scene_dirty = true;
    this->camera_actor_slot = (int8_t)-1;
    GameCanvas::set_speaker_name(std::nullopt);
}

void GameCanvas::follow_actor(int32_t n) {
    if (state().actors[n] == nullptr) {
        return;
    }
    this->camera_actor_slot = (int8_t)n;
    this->scene_dirty = true;
    this->camera_offset[0] = this->canvas_center_x - GameCanvas::state().actors[n]->screen_position[0];
    this->camera_offset[1] = this->canvas_center_y - GameCanvas::state().actors[n]->screen_position[1];
    GameCanvas::set_speaker_name(GameCanvas::state().actors[n]->name);
}

void GameCanvas::update_camera() {
    if (this->camera_actor_slot >= 0 && state().actors[this->camera_actor_slot] != nullptr &&
        GameCanvas::state().actors[this->camera_actor_slot]->is_dead == 0) {
        if (this->center_camera_on_target) {
            int32_t camera_x = this->canvas_center_x - GameCanvas::state().actors[this->camera_actor_slot]->screen_position[0];
            int32_t camera_y = this->canvas_center_y - GameCanvas::state().actors[this->camera_actor_slot]->screen_position[1] +
                               ActorSystem::frame_height(state().actors[this->camera_actor_slot]);
            if (this->camera_offset[0] != camera_x || this->camera_offset[1] != camera_y) {
                this->camera_offset[0] = camera_x;
                this->camera_offset[1] = camera_y;
                this->scene_dirty = true;
            }
            return;
        }
        if (GameCanvas::state().actors[this->camera_actor_slot]->screen_position[1] - ActorSystem::frame_height(state().actors[this->camera_actor_slot]) +
                this->camera_offset[1] <
            0) {
            this->scene_dirty = true;
        } else if (GameCanvas::state().actors[this->camera_actor_slot]->screen_position[1] + this->camera_offset[1] > state().canvas_height) {
            this->scene_dirty = true;
        } else if (GameCanvas::state().actors[this->camera_actor_slot]->screen_position[0] + this->camera_offset[0] < 0) {
            this->scene_dirty = true;
        } else if (GameCanvas::state().actors[this->camera_actor_slot]->screen_position[0] + ActorSystem::frame_width(state().actors[this->camera_actor_slot]) +
                       this->camera_offset[0] >
                   state().canvas_width) {
            this->scene_dirty = true;
        }
        if (this->scene_dirty) {
            this->camera_offset[0] = this->canvas_center_x - GameCanvas::state().actors[this->camera_actor_slot]->screen_position[0];
            this->camera_offset[1] = this->canvas_center_y - GameCanvas::state().actors[this->camera_actor_slot]->screen_position[1] +
                                     ActorSystem::frame_height(state().actors[this->camera_actor_slot]);
        }
    }
}

void GameCanvas::set_tile_scripts(int32_t n, int32_t n2, int32_t n3, int32_t n4, int32_t n5) {
    if (this->tile_enter_script.isNull() || this->tile_exit_script.isNull() || state().tile_use_script.isNull()) {
        return;
    }
    this->tile_enter_script[n * GameCanvas::state().level_cols + n2] = (int8_t)n3;
    this->tile_exit_script[n * GameCanvas::state().level_cols + n2] = (int8_t)n4;
    GameCanvas::state().tile_use_script[n * GameCanvas::state().level_cols + n2] = (int8_t)n5;
}

int32_t GameCanvas::load_resource(const std::string &string) { return current_string_table().load_resource(string); }

void GameCanvas::update_loading_progress(int32_t n) {
    if (state().game_state == 0) {
        return;
    }
    GameCanvas::state().instance->loading_percent = (int8_t)n;
    if (GameCanvas::state().instance->state_before_loading == -1) {
        GameCanvas::state().instance->state_before_loading = state().game_state;
    }
    GameCanvas::set_game_state((int8_t)7);
    state().instance->repaint();
    state().instance->serviceRepaints();
    if (n == 100) {
        GameCanvas::set_game_state(GameCanvas::state().instance->state_before_loading);
        GameCanvas::state().instance->loading_percent = (int8_t)-1;
        GameCanvas::state().instance->state_before_loading = (int8_t)-1;
    }
}

void GameCanvas::show_main_menu() {
    this->build_frontend_menus();
    GameCanvas::set_game_state((int8_t)3);
    state().menu_page = 0;
}

void GameCanvas::set_status_message(const std::optional<std::string> &string, int32_t n, int32_t n2, int32_t n3) {
    state().status_text = string;
    state().status_duration_ms = n * 1000;
    state().status_scroll_style = (int8_t)n3;
    state().status_x = -1;
    state().status_y = -1;
    state().status_elapsed_ms = 0;
    state().status_color = 0;
    state().status_blink_off = false;
    state().status_blink_ms = 0;
    state().status_scroll_ms = 0;
    if (n2 == 0) {
        state().status_color = 0;
    }
    if (n2 == 3) {
        state().status_color = 255;
    }
    if (n2 == 5) {
        state().status_color = 65280;
    }
    if (n2 == 2) {
        state().status_color = 0xFF0000;
    }
    if (n2 == 1) {
        state().status_color = 0xFFFFFF;
    }
    if (n2 == 4) {
        state().status_color = 0xFFFF00;
    }
}

void GameCanvas::load_ui_atlas(const std::string &string) { this->ui_atlas = SpriteAtlas::load_atlas(string); }

void GameCanvas::show_merchant_options() {
    state().menu_page = (int8_t)4;
    GameCanvas::set_game_state((int8_t)3);
}
