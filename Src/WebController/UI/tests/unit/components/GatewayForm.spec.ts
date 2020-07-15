/* tslint:disable no-unused-expression */
import { expect } from 'chai';
import { shallowMount, createLocalVue } from '@vue/test-utils';
import Vuex from 'vuex';

import GatewayForm from '@/components/GatewayForm.vue';
import { modules } from '../store/mockstore';

const localVue = createLocalVue();
localVue.use(Vuex);

describe('@/components/GatewayForm.vue', () => {
  const store = new Vuex.Store({
    modules
  });

  it('GatewayForm is a Vue instance', () => {
    const wrapper = shallowMount(GatewayForm, { store, localVue });
    expect(wrapper.isVueInstance()).to.be.true;
  });
});
