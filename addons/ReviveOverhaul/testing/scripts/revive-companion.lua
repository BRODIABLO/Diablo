return {
    kind = "selector",
    children = {
        {
            kind = "sequence",
            children = {
                { kind = "is_caster" },
                { kind = "owner_distance_lte", distance = 32 },
                { kind = "target_owner_distance_lte", distance = 28 },
                {
                    kind = "selector",
                    children = {
                        {
                            kind = "sequence",
                            children = {
                                { kind = "last_action_cast" },
                                { kind = "target_distance_lte", distance = 14 },
                                { kind = "retreat", distance = 5 },
                            },
                        },
                        {
                            kind = "sequence",
                            children = {
                                { kind = "target_distance_lte", distance = 5 },
                                { kind = "retreat", distance = 6 },
                            },
                        },
                        {
                            kind = "sequence",
                            children = {
                                { kind = "target_distance_gte", distance = 16 },
                                { kind = "chase" },
                            },
                        },
                        {
                            kind = "sequence",
                            children = {
                                { kind = "has_preferred_skill" },
                                { kind = "cast_preferred" },
                            },
                        },
                        { kind = "attack" },
                    },
                },
            },
        },
        {
            kind = "sequence",
            children = {
                { kind = "is_ranged" },
                { kind = "owner_distance_lte", distance = 32 },
                { kind = "target_owner_distance_lte", distance = 28 },
                {
                    kind = "selector",
                    children = {
                        {
                            kind = "sequence",
                            children = {
                                { kind = "last_action_attack" },
                                { kind = "target_distance_lte", distance = 12 },
                                { kind = "retreat", distance = 5 },
                            },
                        },
                        {
                            kind = "sequence",
                            children = {
                                { kind = "target_distance_lte", distance = 5 },
                                { kind = "retreat", distance = 6 },
                            },
                        },
                        {
                            kind = "sequence",
                            children = {
                                { kind = "target_distance_gte", distance = 13 },
                                { kind = "chase" },
                            },
                        },
                        { kind = "attack" },
                    },
                },
            },
        },
        {
            kind = "sequence",
            children = {
                { kind = "is_melee" },
                { kind = "owner_distance_lte", distance = 32 },
                { kind = "target_owner_distance_lte", distance = 28 },
                {
                    kind = "selector",
                    children = {
                        {
                            kind = "sequence",
                            children = {
                                { kind = "target_distance_gte", distance = 4 },
                                { kind = "chase" },
                            },
                        },
                        { kind = "attack" },
                    },
                },
            },
        },
        { kind = "fallback" },
    },
}
