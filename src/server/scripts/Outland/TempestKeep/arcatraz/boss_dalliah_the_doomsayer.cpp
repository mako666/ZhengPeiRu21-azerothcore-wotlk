/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "arcatraz.h"

enum Say
{
    // Dalliah the Doomsayer
    SAY_AGGRO                       = 1,
    SAY_SLAY                        = 2,
    SAY_WHIRLWIND                   = 3,
    SAY_HEAL                        = 4,
    SAY_DEATH                       = 5,
    SAY_SOCCOTHRATES_DEATH          = 7,

    // Wrath-Scryer Soccothrates
    SAY_AGGRO_DALLIAH_FIRST         = 0,
    SAY_DALLIAH_25_PERCENT          = 5
};

enum Spells
{
    SPELL_GIFT_OF_THE_DOOMSAYER     = 36173,
    SPELL_WHIRLWIND                 = 36142,
    SPELL_HEAL                      = 36144,
    SPELL_SHADOW_WAVE               = 39016
};

enum Events
{
    EVENT_GIFT_OF_THE_DOOMSAYER     = 1,
    EVENT_WHIRLWIND                 = 2,
    EVENT_HEAL                      = 3,
    EVENT_SHADOW_WAVE               = 4,
    EVENT_ME_FIRST                  = 5,
    EVENT_SOCCOTHRATES_DEATH        = 6,
    EVENT_CHECK_HEALTH              = 7,
};

class boss_dalliah_the_doomsayer : public CreatureScript
{
public:
    boss_dalliah_the_doomsayer() : CreatureScript("boss_dalliah_the_doomsayer") { }

    struct boss_dalliah_the_doomsayerAI : public BossAI
    {
        boss_dalliah_the_doomsayerAI(Creature* creature) : BossAI(creature, DATA_DALLIAH) { }

        void Reset() override
        {
            _Reset();
            events2.Reset();
            me->SetImmuneToAll(false);
        }

        void InitializeAI() override
        {
            BossAI::InitializeAI();
            if (instance->GetBossState(DATA_SOCCOTHRATES) != DONE)
                me->SetImmuneToAll(true);
        }

        void JustDied(Unit* /*killer*/) override
        {
            _JustDied();
            Talk(SAY_DEATH);

            if (Creature* soccothrates = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SOCCOTHRATES)))
                if (soccothrates->IsAlive() && !soccothrates->IsInCombat())
                    soccothrates->AI()->SetData(1, 1);
        }

        void EnterCombat(Unit* /*who*/) override
        {
            _EnterCombat();
            Talk(SAY_AGGRO);

            events.ScheduleEvent(EVENT_GIFT_OF_THE_DOOMSAYER, urand(1000, 4000));
            events.ScheduleEvent(EVENT_WHIRLWIND, urand(7000, 9000));
            events.ScheduleEvent(EVENT_ME_FIRST, 6000);
            events.ScheduleEvent(EVENT_CHECK_HEALTH, 1000);

            if (IsHeroic())
                events.ScheduleEvent(EVENT_SHADOW_WAVE, urand(11000, 16000));
        }

        void KilledUnit(Unit* victim) override
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_SLAY);
        }

        void SetData(uint32 /*type*/, uint32 data) override
        {
            if (data == 1)
                events2.ScheduleEvent(EVENT_SOCCOTHRATES_DEATH, 6000);
        }

        void UpdateAI(uint32 diff) override
        {
            events2.Update(diff);
            switch (events2.ExecuteEvent())
            {
                case EVENT_SOCCOTHRATES_DEATH:
                    Talk(SAY_SOCCOTHRATES_DEATH);
                    break;
            }

            if (!UpdateVictim())
                return;

            events.Update(diff);
            if (me->HasUnitState(UNIT_STATE_CASTING))
                return;

            switch (events.ExecuteEvent())
            {
                case EVENT_GIFT_OF_THE_DOOMSAYER:
                    me->CastSpell(me->GetVictim(), SPELL_GIFT_OF_THE_DOOMSAYER, false);
                    events.ScheduleEvent(EVENT_GIFT_OF_THE_DOOMSAYER, urand(16000, 21000));
                    break;
                case EVENT_WHIRLWIND:
                    me->CastSpell(me, SPELL_WHIRLWIND, false);
                    Talk(SAY_WHIRLWIND);
                    events.ScheduleEvent(EVENT_WHIRLWIND, urand(19000, 21000));
                    events.ScheduleEvent(EVENT_HEAL, 6000);
                    break;
                case EVENT_HEAL:
                    me->CastSpell(me, SPELL_HEAL, false);
                    Talk(SAY_HEAL);
                    break;
                case EVENT_SHADOW_WAVE:
                    me->CastSpell(me->GetVictim(), SPELL_SHADOW_WAVE, false);
                    events.ScheduleEvent(EVENT_SHADOW_WAVE, urand(11000, 16000));
                    break;
                case EVENT_ME_FIRST:
                    if (Creature* soccothrates = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SOCCOTHRATES)))
                        if (soccothrates->IsAlive() && !soccothrates->IsInCombat())
                            soccothrates->AI()->Talk(SAY_AGGRO_DALLIAH_FIRST);
                    break;
                case EVENT_CHECK_HEALTH:
                    if (HealthBelowPct(25))
                    {
                        if (Creature* soccothrates = ObjectAccessor::GetCreature(*me, instance->GetGuidData(DATA_SOCCOTHRATES)))
                            soccothrates->AI()->Talk(SAY_DALLIAH_25_PERCENT);
                        break;
                    }
                    events.ScheduleEvent(EVENT_CHECK_HEALTH, 1000);
                    break;
            }

            DoMeleeAttackIfReady();
        }

    private:
        EventMap events2;
    };

    CreatureAI* GetAI(Creature* creature) const override
    {
        return GetArcatrazAI<boss_dalliah_the_doomsayerAI>(creature);
    }
};

void AddSC_boss_dalliah_the_doomsayer()
{
    new boss_dalliah_the_doomsayer();
}
