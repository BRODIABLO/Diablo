return {
    kind = "selector",
    children = {
        {
            -- Spellcasters alternate a native MonStats spell with a short
            -- disengage, but only inside the owner's defended combat radius.
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
            -- Physical ranged Revives hold a firing band and kite after each
            -- committed attack instead of wandering or falling back to owner.
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
            -- Melee Revives stay committed to the locked enemy while it
            -- remains inside the owner's pursuit radius.
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
