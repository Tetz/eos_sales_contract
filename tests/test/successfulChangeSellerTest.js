const testService = require('../services/testService');
const { assert } = require('chai');

// TODO: uncomment next line (1) for testing single files
// before(() => testService.before());

beforeEach(() =>  testService.beforeEach());

/***********************************************************************************
 changing Seller functionality
/**********************************************************************************/

describe('Successful changing seller tests', () => {

  /***********************************************************************************
   changeSeller() tests
  /**********************************************************************************/
    
    it('Changing seller and new seller is in the config files', async () => {
      try {
        // When
        await testService.changeSeller('seller', 'random');
        
      } catch (error) {
        // Then
        assert.ifError(error);
      }
      try {
        rowsConfig = await testService.getRowsSaleCon('config')
      } catch (error) {
        assert.ifError(error)
      }
      const config = rowsConfig.rows[0];
      assert.deepEqual(config.seller, 'random');
    })
});